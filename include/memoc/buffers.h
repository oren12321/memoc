#ifndef MEMOC_BUFFERS_H
#define MEMOC_BUFFERS_H

#include <cstdint>
#include <functional>
#include <utility>
#include <type_traits>
#include <concepts>

#include <memoc/blocks.h>
#include <memoc/allocators.h>

namespace memoc {
    namespace details { 
        template <class T, typename U = void>
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
            requires (T t, std::int64_t size, const U* data)
        {
            {T(size, data)} noexcept;
            {t.usable()} noexcept -> std::same_as<bool>;
            {t.data()} noexcept -> std::same_as<Typed_block<U>>;
            {t.init(data)} noexcept -> std::same_as<void>;
        };

        template <std::int64_t Stack_size = 2, bool Lazy_init = false>
        class Stack_buffer {
            static_assert(Stack_size > 0);
        public:
            Stack_buffer() = default;
            Stack_buffer(std::int64_t size, const void* data = nullptr) noexcept
                : size_(size)
            {
                if (!Lazy_init && size_ > 0) {
                    init(data);
                }
            }

            Stack_buffer(const Stack_buffer& other) noexcept
            {
                //if (!other.usable()) {
                //    return;
                //}

                size_ = other.size_;
                data_ = { other.data_.s(), memory_ };
                for (std::int64_t i = 0; i < data_.s(); ++i) {
                    memory_[i] = other.memory_[i];
                }
            }
            Stack_buffer operator=(const Stack_buffer& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }

                //if (!other.usable()) {
                //    return *this;
                //}

                size_ = other.size_;
                data_ = { other.data_.s(), memory_ };
                for (std::int64_t i = 0; i < data_.s(); ++i) {
                    memory_[i] = other.memory_[i];
                }
                return *this;
            }
            Stack_buffer(Stack_buffer&& other) noexcept
                : Stack_buffer(std::cref(other))
            {
                other.data_.clear();
            }
            Stack_buffer& operator=(Stack_buffer&& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }
                operator=(std::cref(other));
                other.data_.clear();
                return *this;
            }
            virtual ~Stack_buffer() = default;

            [[nodiscard]] Block data() const noexcept
            {
                return data_;
            }

            [[nodiscard]] bool usable() const noexcept
            {
                return !data_.empty();
            }

            void init(const void* data = nullptr) noexcept
            {
                if (size_ <= Stack_size) {
                    data_ = { size_, memory_ };
                }

                if (data && !data_.empty()) {
                    const std::uint8_t* bytes = reinterpret_cast<const std::uint8_t*>(data);
                    for (std::int64_t i = 0; i < size_; ++i) {
                        memory_[i] = bytes[i];
                    }
                }
            }

        private:
            std::int64_t size_{ 0 };
            std::uint8_t memory_[Stack_size] = { 0 };
            Block data_{};
        };

        template <Allocator Internal_allocator, bool Lazy_init = false>
        class Allocated_buffer {
        public:
            Allocated_buffer() = default;
            Allocated_buffer(std::int64_t size, const void* data = nullptr) noexcept
                : size_(size)
            {
                if (!Lazy_init && size_ > 0) {
                    init(data);
                }
            }

            Allocated_buffer(const Allocated_buffer& other) noexcept
            {
                //if (!other.usable()) {
                //    return;
                //}

                size_ = other.size_;
                allocator_ = other.allocator_;
                if (!Lazy_init || !other.data_.empty()) {
                    init();
                }
                if (!other.data_.empty()) {
                    const std::uint8_t* src_memory = reinterpret_cast<const std::uint8_t*>(other.data_.p());
                    std::uint8_t* dst_memory = reinterpret_cast<std::uint8_t*>(data_.p());
                    for (std::int64_t i = 0; i < size_; ++i) {
                        dst_memory[i] = src_memory[i];
                    }
                }
            }
            Allocated_buffer operator=(const Allocated_buffer& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }

                //if (!other.usable()) {
                //    return *this;
                //}

                size_ = other.size_;
                allocator_ = other.allocator_;
                if (!Lazy_init || !other.data_.empty()) {
                    allocator_.deallocate(&data_);
                    init();
                }
                if (!other.data_.empty()) {
                    const std::uint8_t* src_memory = reinterpret_cast<const std::uint8_t*>(other.data_.p());
                    std::uint8_t* dst_memory = reinterpret_cast<std::uint8_t*>(data_.p());
                    for (std::int64_t i = 0; i < size_; ++i) {
                        dst_memory[i] = src_memory[i];
                    }
                }
                return *this;
            }
            Allocated_buffer(Allocated_buffer&& other) noexcept
            {
                if (!other.usable()) {
                    return;
                }

                size_ = other.size_;
                allocator_ = std::move(other.allocator_);
                data_ = other.data_;

                other.size_ = 0;
                other.data_.clear();
            }
            Allocated_buffer& operator=(Allocated_buffer&& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }

                if (!other.usable()) {
                    return *this;
                }

                size_ = other.size_;
                allocator_ = std::move(other.allocator_);
                allocator_.deallocate(&data_);
                data_ = other.data_;

                other.size_ = 0;
                other.data_.clear();

                return *this;
            }
            virtual ~Allocated_buffer() noexcept
            {
                if (!data_.empty()) {
                    allocator_.deallocate(&data_);
                }
            }

            [[nodiscard]] Block data() const noexcept
            {
                return data_;
            }

            [[nodiscard]] bool usable() const noexcept
            {
                return !data_.empty();
            }

            void init(const void* data = nullptr) noexcept
            {
                data_ = allocator_.allocate(size_);

                if (data && !data_.empty()) {
                    const std::uint8_t* src_bytes = reinterpret_cast<const std::uint8_t*>(data);
                    std::uint8_t* dst_bytes = reinterpret_cast<std::uint8_t*>(data_.p());
                    for (std::int64_t i = 0; i < size_; ++i) {
                        dst_bytes[i] = src_bytes[i];
                    }
                }
            }

        private:
            std::int64_t size_{ 0 };
            Internal_allocator allocator_{};
            Block data_{};
        };

        template <Buffer Primary, Buffer Fallback>
        class Fallback_buffer
            : private Primary
            // For efficiency should be buffer with lazy initialization
            , private Fallback {
        public:
            Fallback_buffer() = default;
            Fallback_buffer(std::int64_t size, const void* data = nullptr) noexcept
                : Primary(size, data), Fallback(size, data)
            {
                init(data);
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

            [[nodiscard]] Block data() const noexcept
            {
                if (Primary::usable()) {
                    return Primary::data();
                }
                return Fallback::data();
            }

            [[nodiscard]] bool usable() const noexcept
            {
                return Primary::usable() || Fallback::usable();
            }

            void init(const void* data = nullptr) noexcept
            {
                if (!Primary::usable()) {
                    Fallback::init(data);
                }
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
            Typed_buffer() = default;
            Typed_buffer(std::int64_t size, const T* data = nullptr) noexcept
                : Internal_buffer((size * MEMOC_SSIZEOF(Replace_void<T, std::uint8_t>)) / MEMOC_SSIZEOF(Replace_void<Remove_internal_pointer<decltype(Internal_buffer::data().p())>, std::uint8_t>), data) {}

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

            [[nodiscard]] Typed_block<T> data() const noexcept
            {
                return Typed_block<T>{
                    (Internal_buffer::data().s() * MEMOC_SSIZEOF(Replace_void<Remove_internal_pointer<decltype(Internal_buffer::data().p())>, std::uint8_t>)) / MEMOC_SSIZEOF(Replace_void<T, std::uint8_t>),
                    reinterpret_cast<T*>(Internal_buffer::data().p())
                };
            }

            [[nodiscard]] bool usable() const noexcept
            {
                return Internal_buffer::usable();
            }

            void init(const void* data = nullptr) noexcept {}
        };
    }

    using details::Buffer;
    using details::Allocated_buffer;
    using details::Fallback_buffer;
    using details::Stack_buffer;
    using details::Typed_buffer;
}

#endif // MEMOC_BUFFERS_H

