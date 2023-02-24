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
        template <typename T, Allocator Internal_allocator = Malloc_allocator, std::int64_t Prioritized_stack_size = 0>
            requires (!std::is_reference_v<T>)
        class Buffer final {
        public:
            constexpr Buffer(std::int64_t size = 0, const T* data = nullptr)
            {
                ERROC_EXPECT(size >= 0, std::invalid_argument, "invalid buffer size");

                if (size <= Prioritized_stack_size) {
                    block_ = Block<T>(size, reinterpret_cast<T*>(stack_memory_));
                }
                else {
                    Block<void> tmp = allocator_.allocate(size * MEMOC_SSIZEOF(T)).value();
                    block_ = Block<T>(size, reinterpret_cast<T*>(tmp.data()), tmp.hint());
                }

                // For non-fundamental type an object construction is required.
                if constexpr (std::is_fundamental_v<T>) {
                    copy(Block<T>(size, data), block_);
                }
                else {
                    for (std::int64_t i = 0; i < block_.size(); ++i) {
                        memoc::details::construct_at<T>(&(block_[i]));
                    }
                    if (data) {
                        for (std::int64_t i = 0; i < block_.size(); ++i) {
                            block_[i] = data[i];
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

                if (other.block_.size() <= Prioritized_stack_size) {
                    block_ = Block<T>(other.block_.size(), reinterpret_cast<T*>(stack_memory_));
                }
                else {
                    Block<void> tmp = allocator_.allocate(other.block_.size() * MEMOC_SSIZEOF(T)).value();
                    block_ = Block<T>(tmp.size() / MEMOC_SSIZEOF(T), reinterpret_cast<T*>(tmp.data()), tmp.hint());
                }
                copy(other.block_, block_);
            }
            constexpr Buffer operator=(const Buffer& other)
            {
                if (this == &other) {
                    return *this;
                }

                allocator_ = other.allocator_;
                if (block_.size() > Prioritized_stack_size) {
                    Block<void> tmp(block_.size() * MEMOC_SSIZEOF(T), reinterpret_cast<void*>(block_.data()), block_.hint());
                    allocator_.deallocate(tmp);
                }
                block_ = {};

                if (other.empty()) {
                    return *this;
                }

                if (other.size() <= Prioritized_stack_size) {
                    block_ = Block<T>(other.size(), reinterpret_cast<T*>(stack_memory_));
                }
                else {
                    Block<void> tmp = allocator_.allocate(other.block_.size() * MEMOC_SSIZEOF(T)).value();
                    block_ = Block<T>(tmp.size() / MEMOC_SSIZEOF(T), reinterpret_cast<T*>(tmp.data()), tmp.hint());
                }
                copy(other.block_, block_);

                return *this;
            }
            constexpr Buffer(Buffer&& other) noexcept
            {
                if (other.empty()) {
                    return;
                }

                allocator_ = std::move(other.allocator_);
                if (other.size() > Prioritized_stack_size) {
                    block_ = other.block_;
                }
                else {
                    block_ = Block<T>(other.block_.size(), reinterpret_cast<T*>(stack_memory_));
                    copy(other.block_, block_);
                }

                other.block_ = {};
            }
            constexpr Buffer& operator=(Buffer&& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }

                allocator_ = std::move(other.allocator_);
                if (block_.size() > Prioritized_stack_size) {
                    Block<void> tmp(block_.size() * MEMOC_SSIZEOF(T), reinterpret_cast<void*>(block_.data()), block_.hint());
                    allocator_.deallocate(tmp);
                }

                if (other.size() > Prioritized_stack_size) {
                    block_ = other.block_;
                }
                else {
                    block_ = Block<T>(other.block_.size(), reinterpret_cast<T*>(stack_memory_));
                    copy(other.block_, block_);
                }

                other.block_ = {};

                return *this;
            }
            constexpr ~Buffer() noexcept
            {
                if (!block_.empty()) {
                    // For non-fundamental type an object destruction is required.
                    if constexpr (!std::is_fundamental_v<T>) {
                        for (std::int64_t i = 0; i < block_.size(); ++i) {
                            memoc::details::destruct_at<T>(&(block_[i]));
                        }
                    }

                    if (block_.size() > Prioritized_stack_size) {
                        Block<void> tmp(block_.size() * MEMOC_SSIZEOF(T), reinterpret_cast<void*>(block_.data()), block_.hint());
                        allocator_.deallocate(tmp);
                    }
                    block_ = {};
                }
            }

            [[nodiscard]] constexpr Block<T> block() const noexcept
            {
                return block_;
            }

            [[nodiscard]] constexpr bool empty() const noexcept
            {
                return block_.empty();
            }

            [[nodiscard]] constexpr Block<T>::Size_type size() const noexcept
            {
                return block_.size();
            }

            [[nodiscard]] constexpr Block<T>::Pointer data() const noexcept
            {
                return block_.data();
            }

        private:
            Internal_allocator allocator_{};

            inline static constexpr const std::int64_t stack_memory_size_ = Prioritized_stack_size * MEMOC_SSIZEOF(T);
            std::uint8_t stack_memory_[Prioritized_stack_size == 0 ? 1 : stack_memory_size_];

            Block<T> block_{};
        };

        template <Allocator Internal_allocator, std::int64_t Prioritized_stack_size>
        class Buffer<void, Internal_allocator, Prioritized_stack_size> final {
        public:
            constexpr Buffer(std::int64_t size = 0, const void* data = nullptr)
            {
                ERROC_EXPECT(size >= 0, std::invalid_argument, "invalid buffer size");

                if (size <= Prioritized_stack_size) {
                    block_ = Block<void>(size, stack_memory_);
                }
                else {
                    block_ = allocator_.allocate(size).value();
                }
                copy(Block<void>(size, data), block_);
            }

            constexpr Buffer(const Buffer& other)
            {
                allocator_ = other.allocator_;
                if (other.empty()) {
                    return;
                }

                if (other.size() > Prioritized_stack_size) {
                    block_ = allocator_.allocate(other.size()).value();
                }
                else {
                    block_ = Block<void>(other.size(), stack_memory_);
                }
                copy(other.block_, block_);
            }
            constexpr Buffer operator=(const Buffer& other)
            {
                if (this == &other) {
                    return *this;
                }

                allocator_ = other.allocator_;
                if (block_.size() > Prioritized_stack_size) {
                    allocator_.deallocate(block_);
                }

                if (other.empty()) {
                    return *this;
                }

                if (other.size() > Prioritized_stack_size) {
                    block_ = allocator_.allocate(other.size()).value();
                }
                else {
                    block_ = Block<void>(other.size(), stack_memory_);
                }
                copy(other.block_, block_);

                return *this;
            }
            constexpr Buffer(Buffer&& other) noexcept
            {
                if (other.empty()) {
                    return;
                }

                allocator_ = std::move(other.allocator_);
                if (other.size() > Prioritized_stack_size) {
                    block_ = other.block_;
                }
                else {
                    block_ = Block<void>(other.size(), stack_memory_);
                    copy(other.block_, block_);
                }

                other.block_ = {};
            }
            constexpr Buffer& operator=(Buffer&& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }

                allocator_ = std::move(other.allocator_);
                if (block_.size() > Prioritized_stack_size) {
                    allocator_.deallocate(block_);
                }

                if (other.size() > Prioritized_stack_size) {
                    block_ = other.block_;
                }
                else {
                    block_ = Block<void>(other.size(), stack_memory_);
                    copy(other.block_, block_);
                }

                other.block_ = {};

                return *this;
            }
            constexpr ~Buffer() noexcept
            {
                if (!block_.empty() && block_.size() > Prioritized_stack_size) {
                    allocator_.deallocate(block_);
                }
                block_ = {};
            }

            [[nodiscard]] constexpr Block<void> block() const noexcept
            {
                return block_;
            }

            [[nodiscard]] constexpr bool empty() const noexcept
            {
                return block_.empty();
            }

            [[nodiscard]] constexpr Block<void>::Size_type size() const noexcept
            {
                return block_.size();
            }

            [[nodiscard]] constexpr Block<void>::Pointer data() const noexcept
            {
                return block_.data();
            }

        private:
            Internal_allocator allocator_{};

            std::uint8_t stack_memory_[Prioritized_stack_size == 0 ? 1 : Prioritized_stack_size];

            Block<void> block_{};
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
    }

    using details::Buffer;

    using details::create_buffer;
}

#endif // MEMOC_BUFFERS_H

