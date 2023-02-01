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
        template <typename T, Allocator Internal_allocator = Malloc_allocator>
            requires (!std::is_reference_v<T>)
        class Buffer final {
        public:
            constexpr Buffer(std::int64_t size = 0, const T* data = nullptr)
            {
                ERROC_EXPECT(size >= 0, std::invalid_argument, "invalid buffer size");

                Block<void> tmp = allocator_.allocate(size * MEMOC_SSIZEOF(T)).value();
                data_ = Block<T>(size, reinterpret_cast<T*>(tmp.data()), tmp.hint());

                // For non-fundamental type an object construction is required.
                if constexpr (std::is_fundamental_v<T>) {
                    copy(Block<T>(size, data), data_);
                }
                else {
                    for (std::int64_t i = 0; i < data_.size(); ++i) {
                        memoc::details::construct_at<T>(&(data_[i]));
                    }
                    if (data) {
                        for (std::int64_t i = 0; i < data_.size(); ++i) {
                            data_[i] = data[i];
                        }
                    }
                }
            }

            constexpr Buffer(const Buffer& other)
            {
                allocator_ = other.allocator_;
                if (other.empty()) {
                    return;
                }
                {
                    Block<void> tmp = allocator_.allocate(other.data_.size() * MEMOC_SSIZEOF(T)).value();
                    data_ = Block<T>(tmp.size() / MEMOC_SSIZEOF(T), reinterpret_cast<T*>(tmp.data()), tmp.hint());
                }
                copy(other.data_, data_);
            }
            constexpr Buffer operator=(const Buffer& other)
            {
                if (this == &other) {
                    return *this;
                }

                allocator_ = other.allocator_;
                {
                    Block<void> tmp(data_.size() * MEMOC_SSIZEOF(T), reinterpret_cast<void*>(data_.data()), data_.hint());
                    allocator_.deallocate(tmp);
                    data_ = {};
                }

                if (other.empty()) {
                    return *this;
                }
                {
                    Block<void> tmp = allocator_.allocate(other.data_.size() * MEMOC_SSIZEOF(T)).value();
                    data_ = Block<T>(tmp.size() / MEMOC_SSIZEOF(T), reinterpret_cast<T*>(tmp.data()), tmp.hint());
                }
                copy(other.data_, data_);

                return *this;
            }
            constexpr Buffer(Buffer&& other) noexcept
            {
                if (other.empty()) {
                    return;
                }

                allocator_ = std::move(other.allocator_);
                data_ = other.data_;

                other.data_ = {};
            }
            constexpr Buffer& operator=(Buffer&& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }

                allocator_ = std::move(other.allocator_);
                {
                    Block<void> tmp(data_.size() * MEMOC_SSIZEOF(T), reinterpret_cast<void*>(data_.data()), data_.hint());
                    allocator_.deallocate(tmp);
                }
                data_ = other.data_;

                other.data_ = {};

                return *this;
            }
            constexpr ~Buffer() noexcept
            {
                if (!data_.empty()) {
                    // For non-fundamental type an object destruction is required.
                    if constexpr (!std::is_fundamental_v<T>) {
                        for (std::int64_t i = 0; i < data_.size(); ++i) {
                            memoc::details::destruct_at<T>(&(data_[i]));
                        }
                    }

                    {
                        Block<void> tmp(data_.size() * MEMOC_SSIZEOF(T), reinterpret_cast<void*>(data_.data()), data_.hint());
                        allocator_.deallocate(tmp);
                        data_ = {};
                    }
                }
            }

            [[nodiscard]] constexpr Block<T> block() const noexcept
            {
                return data_;
            }

            [[nodiscard]] constexpr bool empty() const noexcept
            {
                return data_.empty();
            }

            [[nodiscard]] constexpr Block<T>::Size_type size() const noexcept
            {
                return data_.size();
            }

            [[nodiscard]] constexpr Block<T>::Pointer data() const noexcept
            {
                return data_.data();
            }

        private:
            Internal_allocator allocator_{};
            Block<T> data_{};
        };

        template <Allocator Internal_allocator>
        class Buffer<void, Internal_allocator> final {
        public:
            constexpr Buffer(std::int64_t size = 0, const void* data = nullptr)
            {
                ERROC_EXPECT(size >= 0, std::invalid_argument, "invalid buffer size");

                data_ = allocator_.allocate(size).value();
                copy(Block<void>(size, data), data_);
            }

            constexpr Buffer(const Buffer& other)
            {
                allocator_ = other.allocator_;
                if (other.empty()) {
                    return;
                }
                data_ = allocator_.allocate(other.size()).value();
                copy(other.data_, data_);
            }
            constexpr Buffer operator=(const Buffer& other)
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
            constexpr Buffer(Buffer&& other) noexcept
            {
                if (other.empty()) {
                    return;
                }

                allocator_ = std::move(other.allocator_);
                data_ = other.data_;

                other.data_ = {};
            }
            constexpr Buffer& operator=(Buffer&& other) noexcept
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
            constexpr ~Buffer() noexcept
            {
                if (!data_.empty()) {
                    allocator_.deallocate(data_);
                }
            }

            [[nodiscard]] constexpr Block<void> block() const noexcept
            {
                return data_;
            }

            [[nodiscard]] constexpr bool empty() const noexcept
            {
                return data_.empty();
            }

            [[nodiscard]] constexpr Block<void>::Size_type size() const noexcept
            {
                return data_.size();
            }

            [[nodiscard]] constexpr Block<void>::Pointer data() const noexcept
            {
                return data_.data();
            }

        private:
            Internal_allocator allocator_{};
            Block<void> data_{};
        };

        template <typename T, Allocator Internal_allocator = Malloc_allocator>
        [[nodiscard]] inline constexpr erroc::Expected<Buffer<T, Internal_allocator>, Buffer_error> create_buffer(std::int64_t size = 0, const T* data = nullptr)
        {
            try {
                return Buffer<T, Internal_allocator>(size, data);
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

        template <typename T = void, Allocator Internal_allocator = Malloc_allocator>
        [[nodiscard]] inline constexpr bool empty(const Buffer<T, Internal_allocator>& buffer) noexcept
        {
            return buffer.empty();
        }

        template <typename T = void, Allocator Internal_allocator = Malloc_allocator>
        [[nodiscard]] inline constexpr auto size(const Buffer<T, Internal_allocator>& buffer) noexcept
        {
            return buffer.size();
        }

        template <typename T = void, Allocator Internal_allocator = Malloc_allocator>
        [[nodiscard]] inline constexpr auto block(const Buffer<T, Internal_allocator>& buffer) noexcept
        {
            return buffer.block();
        }

        template <typename T = void, Allocator Internal_allocator = Malloc_allocator>
        [[nodiscard]] inline constexpr auto data(const Buffer<T, Internal_allocator>& buffer) noexcept
        {
            return buffer.data();
        }
    }

    using details::Buffer;
    using details::create_buffer;
    using details::empty;
    using details::size;
    using details::block;
    using details::data;
}

#endif // MEMOC_BUFFERS_H

