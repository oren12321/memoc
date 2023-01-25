#ifndef MEMOC_BUFFERS_H
#define MEMOC_BUFFERS_H

#include <cstdint>
#include <functional>
#include <utility>
#include <type_traits>
#include <concepts>
#include <stdexcept>

#include <erroc/errors.h>
#include <enumoc/enumoc.h>
#include <memoc/blocks.h>
#include <memoc/allocators.h>
#include <memoc/pointers.h>

ENUMOC_GENERATE(memoc, Buffer_error,
    invalid_size,
    allocator_failure,
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
            {T(size, nullptr)};
            {t.block()} noexcept -> std::same_as<Block<typename decltype(t.block())::Type>>;
            {t.empty()} noexcept -> std::same_as<bool>;
            {t.size()} noexcept -> std::same_as<typename decltype(t.block())::Size_type>;
            {t.data()} noexcept -> std::same_as<typename decltype(t.block())::Pointer>;
        };

        template <std::int64_t Stack_size = 2>
        class Stack_buffer final {
            static_assert(Stack_size > 0);
        public:
            Stack_buffer(std::int64_t size = 0, const void* data = nullptr)
            {
                ERROC_EXPECT(size >= 0 && size <= Stack_size, std::invalid_argument, "invalid buffer size");

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
            ~Stack_buffer() = default;

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
        class Allocated_buffer final {
        public:
            Allocated_buffer(std::int64_t size = 0, const void* data = nullptr)
            {
                ERROC_EXPECT(size >= 0, std::invalid_argument, "invalid buffer size");

                data_ = allocator_.allocate(size).value();
                copy(Block<void>(size, data), data_);
            }

            Allocated_buffer(const Allocated_buffer& other)
            {
                allocator_ = other.allocator_;
                if (other.empty()) {
                    return;
                }
                data_ = allocator_.allocate(other.size()).value();
                copy(other.data_, data_);
            }
            Allocated_buffer operator=(const Allocated_buffer& other)
            {
                if (this == &other) {
                    return *this;
                }

                allocator_ = other.allocator_;
                allocator_.deallocate(data_);

                if (other.empty()) {
                    return *this;
                }
                data_ = allocator_.allocate(other.size()).value();
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
                allocator_.deallocate(data_);
                data_ = other.data_;

                other.data_ = {};

                return *this;
            }
            ~Allocated_buffer() noexcept
            {
                if (!data_.empty()) {
                    allocator_.deallocate(data_);
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
        class Fallback_buffer final {
        public:
            Fallback_buffer(std::int64_t size = 0, const void* data = nullptr)
            {
                try {
                    primary_ = Primary(size, data);
                    use_primary_ = true;
                }
                catch (...) {
                    fallback_ = Fallback(size, data);
                    use_primary_ = false;
                }
            }

            Fallback_buffer(const Fallback_buffer& other)
            {
                use_primary_ = other.use_primary_;
                if (use_primary_) {
                    primary_ = other.primary_;
                }
                else {
                    fallback_ = other.fallback_;
                }
            }
            Fallback_buffer operator=(const Fallback_buffer& other)
            {
                if (this == &other) {
                    return *this;
                }
                use_primary_ = other.use_primary_;
                if (use_primary_) {
                    primary_ = other.primary_;
                    return *this;
                }
                fallback_ = other.fallback_;
                return *this;
            }
            Fallback_buffer(Fallback_buffer&& other) noexcept
            {
                use_primary_ = other.use_primary_;
                if (use_primary_) {
                    primary_ = std::move(other.primary_);
                }
                else {
                    fallback_ = std::move(other.fallback_);
                }
            }
            Fallback_buffer& operator=(Fallback_buffer&& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }
                use_primary_ = other.use_primary_;
                if (use_primary_) {
                    primary_ = std::move(other.primary_);
                    return *this;
                }
                fallback_ = std::move(other.fallback_);
                return *this;
            }
            ~Fallback_buffer()
            {
                if (use_primary_) {
                    primary_.~Primary();
                }
                else {
                    fallback_.~Fallback();
                }
            }

            [[nodiscard]] Block<void> block() const noexcept
            {
                return use_primary_ ? primary_.block() : fallback_.block();
            }

            [[nodiscard]] bool empty() const noexcept
            {
                return use_primary_ ? primary_.empty() : fallback_.empty();
            }

            [[nodiscard]] Block<void>::Size_type size() const noexcept
            {
                return use_primary_ ? primary_.size() : fallback_.size();
            }

            [[nodiscard]] Block<void>::Pointer data() const noexcept
            {
                return use_primary_ ? primary_.data() : fallback_.data();
            }

        private:
            static constexpr std::int64_t mem_size_ = MEMOC_SSIZEOF(Primary) > MEMOC_SSIZEOF(Fallback) ? MEMOC_SSIZEOF(Primary) : MEMOC_SSIZEOF(Fallback);

            union {
                Primary primary_;
                Fallback fallback_;
                std::uint8_t memory_[mem_size_]{ 0 };
            };

            bool use_primary_{ true };
        };

        template <typename T, Buffer Internal_buffer>
            requires (!std::is_reference_v<T>)
        class Typed_buffer final {
            // If required or internal type is void then sizeof is invalid - replace it with byte size
            template <typename U_src, typename U_dst>
            using Replace_void = std::conditional_t<std::is_same<U_src, void>::value, U_dst, U_src>;

            template <typename U>
            using Remove_internal_pointer = typename std::remove_pointer_t<U>;
        public:
            Typed_buffer(std::int64_t size = 0, const T* data = nullptr)
                : internal_(size > 0 ? (size* MEMOC_SSIZEOF(Replace_void<T, std::uint8_t>)) / MEMOC_SSIZEOF(Replace_void<Remove_internal_pointer<decltype(memoc::data(internal_.block()))>, std::uint8_t>) : size, data)
            {
                if (internal_.empty()) {
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

            Typed_buffer(const Typed_buffer& other)
                : internal_(other.internal_) {}
            Typed_buffer operator=(const Typed_buffer& other)
            {
                if (this == &other) {
                    return *this;
                }
                internal_ = other.internal_;
                return *this;
            }
            Typed_buffer(Typed_buffer&& other) noexcept
                : internal_(std::move(other.internal_)) {}
            Typed_buffer& operator=(Typed_buffer&& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }
                internal_ = std::move(other.internal_);
                return *this;
            }
            virtual ~Typed_buffer() = default;

            [[nodiscard]] Block<T> block() const noexcept
            {
                return Block<T>{
                    (internal_.size() * MEMOC_SSIZEOF(Replace_void<Remove_internal_pointer<decltype(internal_.data())>, std::uint8_t>)) / MEMOC_SSIZEOF(Replace_void<T, std::uint8_t>),
                    reinterpret_cast<T*>(internal_.data())
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

        private:
            Internal_buffer internal_;
        };

        template <Buffer T, typename U = void>
        [[nodiscard]] inline erroc::Expected<T, Buffer_error> create(std::int64_t size = 0, const typename decltype(T().block())::Type* data = nullptr)
        {
            try {
                return T(size, data);
            }
            catch (const std::invalid_argument&) {
                return erroc::Unexpected(Buffer_error::invalid_size);
            }
            catch (const std::runtime_error&) {
                return erroc::Unexpected(Buffer_error::allocator_failure);
            }
            catch (...) {
                return erroc::Unexpected(Buffer_error::unknown);
            }
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

