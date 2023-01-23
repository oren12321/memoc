#ifndef MEMOC_BLOCKS_H
#define MEMOC_BLOCKS_H

#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>
#include <cassert>

namespace memoc {
    namespace details {
        template <std::uint64_t U>
        [[nodiscard]] constexpr std::int64_t safe_64_unsigned_to_signed_cast() noexcept
        {
            static_assert(U <= std::numeric_limits<std::int64_t>::max(), "unsgined value is too large for casting to its unsgined version");
            return static_cast<std::int64_t>(U);
        }

        [[nodiscard]] constexpr std::int64_t safe_64_unsigned_to_signed_cast(std::uint64_t u) noexcept
        {
            assert((u <= std::numeric_limits<std::int64_t>::max()) && "unsgined value is too large for casting to its unsgined version");
            return static_cast<std::int64_t>(u);
        }
    }
}

#define MEMOC_SSIZEOF(...) memoc::details::safe_64_unsigned_to_signed_cast<sizeof(__VA_ARGS__)>()

namespace memoc {
    namespace details {
        template <typename T>
            requires (!std::is_reference_v<T>)
        class Block {
        public:
            using Size_type = std::int64_t;
            using Type = T;
            using Pointer = T*;
            using Const_pointer = const T*;

            Block(const Block&) noexcept = default;
            Block& operator=(const Block&) noexcept = default;
            Block(Block&&) noexcept = default;
            Block& operator=(Block&&) noexcept = default;
            virtual ~Block() noexcept = default;

            // Do not allow parially empty block
            Block(Size_type s = 0, Const_pointer p = nullptr) noexcept
                : s_(p ? (s > 0 ? s : 0) : 0), p_(s > 0 ? const_cast<Pointer>(p) : nullptr)
            {
            }

            [[nodiscard]] bool empty() const noexcept
            {
                return !s_ && !p_;
            }

            [[nodiscard]] Size_type size() const noexcept
            {
                return s_;
            }

            Pointer data() const noexcept
            {
                return p_;
            }

            [[nodiscard]] const Type& operator[](std::int64_t offset) const noexcept
            {
                return p_[offset];
            }

            Type& operator[](std::int64_t offset) noexcept
            {
                return p_[offset];
            }

        private:
            Size_type s_{ 0 };
            Pointer p_{ nullptr };
        };

        template <typename T>
        [[nodiscard]] inline bool empty(const Block<T>& b) noexcept
        {
            return b.empty();
        }

        template <typename T>
        [[nodiscard]] inline std::int64_t size(const Block<T>& b) noexcept
        {
            return b.size();
        }

        template <typename T>
        [[nodiscard]] Block<T>::Pointer data(const Block<T>& b) noexcept
        {
            return b.data();
        }

        template <typename T1, typename T2>
        [[nodiscard]] inline bool operator==(const Block<T1>& lhs, const Block<T2>& rhs) noexcept
        {
            if (empty(lhs) && empty(rhs)) {
                return true;
            }

            if (size(lhs) != size(rhs)) {
                return false;
            }

            if (size(lhs) == 0) {
                return false;
            }

            bool still_equal{ true };
            for (std::int64_t i = 0; i < size(lhs) && still_equal; ++i) {
                still_equal &= (data(lhs)[i] == data(rhs)[i]);
            }
            return still_equal;
        }

        template <typename T1, typename T2>
        inline std::int64_t copy(const Block<T1>& src, Block<T2> dst, std::int64_t count) noexcept
        {
            std::int64_t min_size{ size(src) > size(dst) ? size(dst) : size(src) };
            std::int64_t num_copied{ count > min_size ? min_size : count };
            if (num_copied == 0) {
                return 0;
            }

            for (std::int64_t i = 0; i < num_copied; ++i) {
                data(dst)[i] = data(src)[i];
            }
            return num_copied;
        }

        template <typename T1, typename T2>
        inline std::int64_t copy(const Block<T1>& src, Block<T2> dst) noexcept
        {
            return copy(src, dst, size(src));
        }

        template <typename T1, typename T2>
        inline std::int64_t set(Block<T1> b, const T2& value, std::int64_t count) noexcept
        {
            std::int64_t num_set{ count > size(b) ? size(b) : count };
            if (num_set == 0) {
                return 0;
            }

            for (std::int64_t i = 0; i < num_set; ++i) {
                data(b)[i] = value;
            }
            return num_set;
        }

        template <typename T1, typename T2>
        inline std::int64_t set(Block<T1> b, const T2& value) noexcept
        {
            return set(b, value, size(b));
        }

        template <>
        class Block<void> {
        public:
            using Size_type = std::int64_t;
            using Type = void;
            using Pointer = void*;
            using Const_pointer = const void*;

            Block(const Block&) noexcept = default;
            Block& operator=(const Block&) noexcept = default;
            Block(Block&&) noexcept = default;
            Block& operator=(Block&&) noexcept = default;
            virtual ~Block() noexcept = default;

            // Do not allow parially empty block
            Block(Size_type s = 0, Const_pointer p = nullptr) noexcept
                : s_(p ? (s > 0 ? s : 0) : 0), p_(s > 0 ? const_cast<Pointer>(p) : nullptr)
            {
            }

            [[nodiscard]] bool empty() const noexcept
            {
                return !s_ && !p_;
            }

            [[nodiscard]] Size_type size() const noexcept
            {
                return s_;
            }

            Pointer data() const noexcept
            {
                return p_;
            }

        private:
            Size_type s_{ 0 };
            Pointer p_{ nullptr };
        };

        [[nodiscard]] inline bool operator==(const Block<void>& lhs, const Block<void>& rhs) noexcept
        {
            if (empty(lhs) && empty(rhs)) {
                return true;
            }

            if (size(lhs) != size(rhs)) {
                return false;
            }

            if (size(lhs) == 0) {
                return false;
            }

            const std::uint8_t* lhs_ptr{ reinterpret_cast<const std::uint8_t*>(data(lhs)) };
            const std::uint8_t* rhs_ptr{ reinterpret_cast<const std::uint8_t*>(data(rhs)) };

            bool still_equal{ true };
            for (std::int64_t i = 0; i < size(lhs) && still_equal; ++i) {
                still_equal &= (lhs_ptr[i] == rhs_ptr[i]);
            }
            return still_equal;
        }

        template <typename T>
        [[nodiscard]] inline bool operator==(const Block<void>& lhs, const Block<T>& rhs) noexcept
        {
            return operator==(lhs, Block<void>{MEMOC_SSIZEOF(T)* size(rhs), data(rhs)});
        }

        template <typename T>
        [[nodiscard]] inline bool operator==(const Block<T>& lhs, const Block<void>& rhs) noexcept
        {
            return operator==(Block<void>{MEMOC_SSIZEOF(T)* size(lhs), data(lhs)}, rhs);
        }

        inline std::int64_t copy(const Block<void>& src, Block<void> dst, std::int64_t bytes) noexcept
        {
            std::int64_t min_size{ size(src) > size(dst) ? size(dst) : size(src) };
            std::int64_t num_copied{ bytes > min_size ? min_size : bytes };
            if (num_copied == 0) {
                return 0;
            }

            const std::uint8_t* src_ptr{ reinterpret_cast<const std::uint8_t*>(data(src)) };
            std::uint8_t* dst_ptr{ reinterpret_cast<std::uint8_t*>(data(dst)) };

            for (std::int64_t i = 0; i < num_copied; ++i) {
                dst_ptr[i] = src_ptr[i];
            }
            return num_copied;
        }

        inline std::int64_t copy(const Block<void>& src, Block<void> dst) noexcept
        {
            return copy(src, dst, size(src));
        }

        template <typename T>
        inline std::int64_t copy(const Block<void>& src, Block<T> dst, std::int64_t bytes) noexcept
        {
            return copy(src, Block<void>{MEMOC_SSIZEOF(T)* size(dst), data(dst)}, bytes);
        }

        template <typename T>
        inline std::int64_t copy(const Block<void>& src, Block<T> dst) noexcept
        {
            return copy(src, Block<void>{MEMOC_SSIZEOF(T)* size(dst), data(dst)}, size(src));
        }

        template <typename T>
        inline std::int64_t copy(const Block<T>& src, Block<void> dst, std::int64_t bytes) noexcept
        {
            return copy(Block<void>{MEMOC_SSIZEOF(T)* size(src), data(src)}, dst, bytes);
        }

        template <typename T>
        inline std::int64_t copy(const Block<T>& src, Block<void> dst) noexcept
        {
            return copy(Block<void>{MEMOC_SSIZEOF(T)* size(src), data(src)}, dst, MEMOC_SSIZEOF(T)* size(src));
        }

        template <typename T>
        inline std::int64_t set(Block<void> b, const T& value, std::int64_t count) noexcept
        {
            std::int64_t block_size_by_type{ size(b) / MEMOC_SSIZEOF(T) };
            std::int64_t num_set{ count > block_size_by_type ? block_size_by_type : count };
            if (num_set == 0) {
                return 0;
            }

            T* ptr{ reinterpret_cast<T*>(data(b)) };
            for (std::int64_t i = 0; i < num_set; ++i) {
                ptr[i] = value;
            }
            return num_set;
        }

        template <typename T>
        inline std::int64_t set(Block<void> b, const T& value) noexcept
        {
            return set(b, value, size(b) / MEMOC_SSIZEOF(T));
        }
    }

    using details::Block;

    using details::empty;
    using details::size;
    using details::data;
    using details::copy;
    using details::set;
}

#endif // MEMOC_BLOCKS_H