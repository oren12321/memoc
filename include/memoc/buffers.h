#ifndef MEMOC_BUFFERS_H
#define MEMOC_BUFFERS_H

#include <cstdint>
#include <functional>
#include <utility>
#include <type_traits>
#include <concepts>

#include <erroc/errors.h>
#include <enumoc/enumoc.h>
#include <memoc/blocks.h>
#include <memoc/allocators.h>
#include <memoc/pointers.h>

ENUMOC_GENERATE(memoc, Buffer_error,
    invalid_size,
    unknown);

namespace memoc {
    namespace details { 
        template <class T>
        concept Buffer = 
            requires
        {
            std::is_default_constructible_v<T>;
            std::is_copy_constructible_v<T>;
            std::is_copy_assignable_v<T>;
            std::is_move_constructible_v<T>;
            std::is_move_assignable_v<T>;
            std::is_destructible_v<T>;
        }&&
            requires (T t, std::int64_t size)
        {
            {T(size, nullptr)} noexcept;
            {t.block()} noexcept -> std::same_as<Block<typename decltype(t.block())::Type>>;
            {t.empty()} noexcept -> std::same_as<bool>;
            {t.size()} noexcept -> std::same_as<typename decltype(t.block())::Size_type>;
            {t.data()} noexcept -> std::same_as<typename decltype(t.block())::Pointer>;
        };

        template <std::int64_t Stack_size = 2>
        class Stack_buffer {
            static_assert(Stack_size > 0);
        public:
            Stack_buffer(std::int64_t size = 0, const void* data = nullptr) noexcept
            {
                if (size <= 0 || size > Stack_size) {
                    return;
                }
                data_ = { size, memory_ };
                copy(Block<void>(size, data), data_);
            }

            Stack_buffer(const Stack_buffer& other) noexcept
            {
                if (other.empty()) {
                    return;
                }

                const std::int64_t size{ other.size() };
                data_ = { size, memory_ };
                copy(Block<void>(size, other.memory_), data_);
            }
            Stack_buffer operator=(const Stack_buffer& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }

                const std::int64_t size{ other.size() };
                data_ = { size, memory_ };

                if (data_.empty()) {
                    return *this;
                }

                copy(Block<void>(size, other.memory_), data_);

                return *this;
            }
            Stack_buffer(Stack_buffer&& other) noexcept
                : Stack_buffer(std::cref(other))
            {
                other.data_ = {};
            }
            Stack_buffer& operator=(Stack_buffer&& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }
                operator=(std::cref(other));
                other.data_ = {};
                return *this;
            }
            virtual ~Stack_buffer() = default;

            [[nodiscard]] Block<void> block() const noexcept
            {
                return data_;
            }

            [[nodiscard]] bool empty() const noexcept
            {
                return data_.empty();
            }

            [[nodiscard]] Block<void>::Size_type size() const noexcept
            {
                return data_.size();
            }

            [[nodiscard]] Block<void>::Pointer data() const noexcept
            {
                return data_.data();
            }

        private:
            std::uint8_t memory_[Stack_size] = { 0 };
            Block<void> data_{};
        };

        template <Allocator Internal_allocator>
        class Allocated_buffer {
        public:
            Allocated_buffer(std::int64_t size = 0, const void* data = nullptr) noexcept
            {
                if (size <= 0) {
                    return;
                }
                data_ = allocator_.allocate(size);
                copy(Block<void>(size, data), data_);
            }

            Allocated_buffer(const Allocated_buffer& other) noexcept
            {
                allocator_ = other.allocator_;
                if (other.empty()) {
                    return;
                }
                data_ = allocator_.allocate(other.size());
                copy(other.data_, data_);
            }
            Allocated_buffer operator=(const Allocated_buffer& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }

                allocator_ = other.allocator_;
                allocator_.deallocate(&data_);

                if (other.empty()) {
                    return *this;
                }
                data_ = allocator_.allocate(other.size());
                copy(other.data_, data_);

                return *this;
            }
            Allocated_buffer(Allocated_buffer&& other) noexcept
            {
                if (other.empty()) {
                    return;
                }

                allocator_ = std::move(other.allocator_);
                data_ = other.data_;

                other.data_ = {};
            }
            Allocated_buffer& operator=(Allocated_buffer&& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }

                allocator_ = std::move(other.allocator_);
                allocator_.deallocate(&data_);
                data_ = other.data_;

                other.data_ = {};

                return *this;
            }
            virtual ~Allocated_buffer() noexcept
            {
                if (!data_.empty()) {
                    allocator_.deallocate(&data_);
                }
            }

            [[nodiscard]] Block<void> block() const noexcept
            {
                return data_;
            }

            [[nodiscard]] bool empty() const noexcept
            {
                return data_.empty();
            }

            [[nodiscard]] Block<void>::Size_type size() const noexcept
            {
                return data_.size();
            }

            [[nodiscard]] Block<void>::Pointer data() const noexcept
            {
                return data_.data();
            }

        private:
            Internal_allocator allocator_{};
            Block<void> data_{};
        };

        template <Buffer Primary, Buffer Fallback>
        class Fallback_buffer
            : private Primary
            // For efficiency should be buffer with lazy initialization
            , private Fallback {
        public:
            Fallback_buffer(std::int64_t size = 0, const void* data = nullptr) noexcept
                : Primary(size, data)
            {
                if (Primary::empty()) {
                    *(dynamic_cast<Fallback*>(this)) = Fallback(size, data);
                }
            }

            Fallback_buffer(const Fallback_buffer& other) noexcept
                : Primary(other), Fallback(other) {}
            Fallback_buffer operator=(const Fallback_buffer& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }
                Primary::operator=(other);
                Fallback::operator=(other);
                return *this;
            }
            Fallback_buffer(Fallback_buffer&& other) noexcept
                : Primary(std::move(other)), Fallback(std::move(other)) {}
            Fallback_buffer& operator=(Fallback_buffer&& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }
                Primary::operator=(std::move(other));
                Fallback::operator=(std::move(other));
                return *this;
            }
            virtual ~Fallback_buffer() = default;

            [[nodiscard]] Block<void> block() const noexcept
            {
                if (!Primary::empty()) {
                    return Primary::block();
                }
                return Fallback::block();
            }

            [[nodiscard]] bool empty() const noexcept
            {
                return Primary::empty() && Fallback::empty();
            }

            [[nodiscard]] Block<void>::Size_type size() const noexcept
            {
                if (!Primary::empty()) {
                    return Primary::size();
                }
                return Fallback::size();
            }

            [[nodiscard]] Block<void>::Pointer data() const noexcept
            {
                if (!Primary::empty()) {
                    return Primary::data();
                }
                return Fallback::data();
            }
        };

        template <typename T, Buffer Internal_buffer>
            requires (!std::is_reference_v<T>)
        class Typed_buffer : private Internal_buffer {
            // If required or internal type is void then sizeof is invalid - replace it with byte size
            template <typename U_src, typename U_dst>
            using Replace_void = std::conditional_t<std::is_same<U_src, void>::value, U_dst, U_src>;

            template <typename U>
            using Remove_internal_pointer = typename std::remove_pointer_t<U>;
        public:
            Typed_buffer(std::int64_t size = 0, const T* data = nullptr) noexcept
                : Internal_buffer(size > 0 ? (size* MEMOC_SSIZEOF(Replace_void<T, std::uint8_t>)) / MEMOC_SSIZEOF(Replace_void<Remove_internal_pointer<decltype(memoc::data(Internal_buffer::block()))>, std::uint8_t>) : size, data)
            {
                if (Internal_buffer::empty()) {
                    return;
                }

                // For non-fundamental type an object construction is required.
                if (!std::is_fundamental_v<T>) {
                    Block<T> b{ this->block() };
                    for (std::int64_t i = 0; i < memoc::size(b); ++i) {
                        memoc::details::construct_at<T>(reinterpret_cast<T*>(&(b[i])));
                    }
                    if (data) {
                        const T* typed_data = reinterpret_cast<const T*>(data);
                        for (std::int64_t i = 0; i < memoc::size(b); ++i) {
                            b[i] = typed_data[i];
                        }
                    }
                }
            }

            Typed_buffer(const Typed_buffer& other) noexcept
                : Internal_buffer(other) {}
            Typed_buffer operator=(const Typed_buffer& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }
                Internal_buffer::operator=(other);
                return *this;
            }
            Typed_buffer(Typed_buffer&& other) noexcept
                : Internal_buffer(std::move(other)) {}
            Typed_buffer& operator=(Typed_buffer&& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }
                Internal_buffer::operator=(std::move(other));
                return *this;
            }
            virtual ~Typed_buffer() = default;

            [[nodiscard]] Block<T> block() const noexcept
            {
                return Block<T>{
                    (Internal_buffer::size() * MEMOC_SSIZEOF(Replace_void<Remove_internal_pointer<decltype(Internal_buffer::data())>, std::uint8_t>)) / MEMOC_SSIZEOF(Replace_void<T, std::uint8_t>),
                    reinterpret_cast<T*>(Internal_buffer::data())
                };
            }

            [[nodiscard]] bool empty() const noexcept
            {
                return block().empty();
            }

            [[nodiscard]] Block<T>::Size_type size() const noexcept
            {
                return block().size();
            }

            [[nodiscard]] Block<T>::Pointer data() const noexcept
            {
                return block().data();
            }


        };

        template <Buffer T, typename U = void>
        [[nodiscard]] inline erroc::Expected<T, Buffer_error> create(std::int64_t size = 0, const typename decltype(T().block())::Type* data = nullptr)
        {
            if (size < 0) {
                return erroc::Unexpected(Buffer_error::invalid_size);
            }

            if (size == 0) {
                return T();
            }

            T buff(size, data);
            if (buff.empty()) {
                return erroc::Unexpected(Buffer_error::unknown);
            }
            return buff;
        }

        template <Buffer T>
        [[nodiscard]] inline bool empty(const T& buffer) noexcept
        {
            return buffer.empty();
        }

        template <Buffer T>
        [[nodiscard]] inline auto size(const T& buffer) noexcept
            -> decltype(buffer.size())
        {
            return buffer.size();
        }

        template <Buffer T>
        [[nodiscard]] inline auto block(const T& buffer) noexcept
            -> decltype(buffer.block())
        {
            return buffer.block();
        }

        template <Buffer T>
        [[nodiscard]] inline auto data(const T& buffer) noexcept
            -> decltype(buffer.data())
        {
            return buffer.data();
        }
    }

    using details::Buffer;
    using details::Allocated_buffer;
    using details::Fallback_buffer;
    using details::Stack_buffer;
    using details::Typed_buffer;
    using details::create;
    using details::empty;
    using details::size;
    using details::block;
    using details::data;
}

#endif // MEMOC_BUFFERS_H

