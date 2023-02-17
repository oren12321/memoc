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
        struct Block2 final {
            std::int64_t size{ 0 };
            T* data{ nullptr };
            std::int64_t hint{ std::numeric_limits<std::int64_t>::min() };
        };

        template <typename T>
        [[nodiscard]] constexpr inline bool empty(const Block2<T>& b) noexcept
        {
            return !b.size || !b.data;
        }

        template <typename T, typename U>
        [[nodiscard]] inline constexpr bool operator==(const Block2<T>& lhs, const Block2<U>& rhs) noexcept
        {
            if (empty(lhs) && empty(rhs)) {
                return true;
            }

            if (lhs.size != rhs.size) {
                return false;
            }

            for (std::int64_t i = 0; i < lhs.size; ++i) {
                if (lhs.data[i] != rhs.data[i]) {
                    return false;
                }
            }
            return true;
        }

        template <typename T, typename U>
            requires (std::is_same_v<void, std::remove_const_t<T>> || std::is_same_v<void, std::remove_const_t<U>>)
        [[nodiscard]] inline constexpr bool operator==(const Block2<T>& lhs, const Block2<U>& rhs) noexcept
        {
            if (empty(lhs) && empty(rhs)) {
                return true;
            }

            const std::int64_t lhs_bytes_size = MEMOC_SSIZEOF(std::conditional_t<std::is_same_v<void, std::remove_const_t<T>>, std::uint8_t, T>) * lhs.size;
            const std::int64_t rhs_bytes_size = MEMOC_SSIZEOF(std::conditional_t<std::is_same_v<void, std::remove_const_t<U>>, std::uint8_t, U>) * rhs.size;

            if (lhs_bytes_size != rhs_bytes_size) {
                return false;
            }

            const std::uint8_t* lhs_ptr{ reinterpret_cast<const std::uint8_t*>(lhs.data) };
            const std::uint8_t* rhs_ptr{ reinterpret_cast<const std::uint8_t*>(rhs.data) };

            for (std::int64_t i = 0; i < lhs_bytes_size; ++i) {
                if (lhs_ptr[i] != rhs_ptr[i]) {
                    return false;
                }
            }
            return true;
        }

        template <typename T, typename U>
        [[nodiscard]] constexpr inline std::int64_t copy(const Block2<T>& src, Block2<U> dst, std::int64_t count = std::numeric_limits<std::int64_t>::max()) noexcept
        {
            const std::int64_t min_size{ src.size < dst.size ? src.size : dst.size };
            const std::int64_t num_copied{ count < min_size ? count : min_size };

            for (std::int64_t i = 0; i < num_copied; ++i) {
                dst.data[i] = src.data[i];
            }

            return num_copied;
        }

        template <typename T, typename U>
            requires (std::is_same_v<void, std::remove_const_t<T>> || std::is_same_v<void, std::remove_const_t<U>>)
        [[nodiscard]] constexpr inline std::int64_t copy(const Block2<T>& src, Block2<U> dst, std::int64_t bytes = std::numeric_limits<std::int64_t>::max()) noexcept
        {
            const std::int64_t src_bytes_count = MEMOC_SSIZEOF(std::conditional_t<std::is_same_v<void, std::remove_const_t<T>>, std::uint8_t, T>) * src.size;
            const std::int64_t dst_bytes_count = MEMOC_SSIZEOF(std::conditional_t<std::is_same_v<void, std::remove_const_t<U>>, std::uint8_t, U>) * dst.size;

            const std::int64_t min_size{ src_bytes_count > dst_bytes_count ? dst_bytes_count : src_bytes_count };
            const std::int64_t num_copied{ bytes > min_size ? min_size : bytes };

            const std::uint8_t* src_ptr{ reinterpret_cast<const std::uint8_t*>(src.data) };
            std::uint8_t* dst_ptr{ reinterpret_cast<std::uint8_t*>(dst.data) };

            for (std::int64_t i = 0; i < num_copied; ++i) {
                dst_ptr[i] = src_ptr[i];
            }

            return num_copied;
        }

        template <typename T, typename U>
        [[nodiscard]] constexpr inline std::int64_t copy(const T& value, Block2<U> dst, std::int64_t count = std::numeric_limits<std::int64_t>::max()) noexcept
        {
            const std::int64_t num_copied{ count < dst.size ? count : dst.size };

            for (std::int64_t i = 0; i < num_copied; ++i) {
                dst.data[i] = value;
            }

            return num_copied;
        }

        template <typename T>
        [[nodiscard]] constexpr inline std::int64_t copy(const T& value, Block2<void> dst, std::int64_t bytes = std::numeric_limits<std::int64_t>::max()) noexcept
        {
            constexpr const std::int64_t value_bytes_size = MEMOC_SSIZEOF(T);

            const std::int64_t num_bytes_copied{ bytes < dst.size ? bytes : dst.size };

            const std::int64_t num_full_copied{ num_bytes_copied / value_bytes_size };

            T* dst_T_ptr{ reinterpret_cast<T*>(dst.data) };

            for (std::int64_t i = 0; i < num_full_copied; ++i) {
                dst_T_ptr[i] = value;
            }

            const std::int64_t num_remained_bytes_copied{ num_bytes_copied - num_full_copied * value_bytes_size };

            std::uint8_t* dst_bytes_ptr{ reinterpret_cast<std::uint8_t*>(dst.data) + num_full_copied * value_bytes_size };
            std::uint8_t* value_bytes_ptr{ reinterpret_cast<std::uint8_t*>(value) };

            for (std::int64_t i = 0; i < num_remained_bytes_copied; ++i) {
                dst_bytes_ptr[i] = value_bytes_ptr[i];
            }

            return num_bytes_copied;
        }

        template <typename T>
            requires (!std::is_reference_v<T>)
        class Block final {
        public:
            using Size_type = std::int64_t;
            using Type = T;
            using Pointer = T*;
            using Const_pointer = const T*;

            constexpr Block(const Block&) noexcept = default;
            constexpr Block& operator=(const Block&) noexcept = default;
            constexpr Block(Block&&) noexcept = default;
            constexpr Block& operator=(Block&&) noexcept = default;
            constexpr ~Block() noexcept = default;

            // Do not allow parially empty block
            constexpr Block(Size_type s = 0, Const_pointer p = nullptr, std::int64_t hint = std::numeric_limits<std::int64_t>::min()) noexcept
                : s_(p ? (s > 0 ? s : 0) : 0), p_(s > 0 ? const_cast<Pointer>(p) : nullptr), hint_(hint)
            {
            }

            [[nodiscard]] constexpr bool empty() const noexcept
            {
                return !s_ && !p_;
            }

            [[nodiscard]] constexpr Size_type size() const noexcept
            {
                return s_;
            }

            constexpr Pointer data() const noexcept
            {
                return p_;
            }

            [[nodiscard]] constexpr const Type& operator[](std::int64_t offset) const noexcept
            {
                return p_[offset];
            }

            constexpr Type& operator[](std::int64_t offset) noexcept
            {
                return p_[offset];
            }

            [[nodiscard]] constexpr std::int64_t hint() const noexcept
            {
                return hint_;
            }

        private:
            Size_type s_{ 0 };
            Pointer p_{ nullptr };
            std::int64_t hint_{ std::numeric_limits<std::int64_t>::min() };
        };

        template <typename T>
        [[nodiscard]] inline constexpr bool empty(const Block<T>& b) noexcept
        {
            return b.empty();
        }

        template <typename T>
        [[nodiscard]] inline constexpr std::int64_t size(const Block<T>& b) noexcept
        {
            return b.size();
        }

        template <typename T>
        [[nodiscard]] constexpr Block<T>::Pointer data(const Block<T>& b) noexcept
        {
            return b.data();
        }

        template <typename T1, typename T2>
        [[nodiscard]] inline constexpr bool operator==(const Block<T1>& lhs, const Block<T2>& rhs) noexcept
        {
            const std::int64_t lhs_size{ lhs.size() };
            if (lhs_size != rhs.size()) {
                return false;
            }

            if (lhs.empty()) {
                return true;
            }

            for (std::int64_t i = 0; i < lhs_size; ++i) {
                if (lhs[i] != rhs[i]) {
                    return false;
                }
            }
            return true;
        }

        template <typename T1, typename T2>
        inline std::int64_t constexpr copy(const Block<T1>& src, Block<T2> dst, std::int64_t count) noexcept
        {
            if (count == 0) {
                return 0;
            }

            if (src.empty() || dst.empty()) {
                return 0;
            }

            const std::int64_t min_size{ src.size() > dst.size() ? dst.size() : src.size() };
            const std::int64_t num_copied{ count > min_size ? min_size : count };

            for (std::int64_t i = 0; i < num_copied; ++i) {
                dst.data()[i] = src.data()[i];
            }
            return num_copied;
        }

        template <typename T1, typename T2>
        inline constexpr std::int64_t copy(const Block<T1>& src, Block<T2> dst) noexcept
        {
            return copy(src, dst, src.size());
        }

        template <typename T1, typename T2>
        inline constexpr std::int64_t set(Block<T1> b, const T2& value, std::int64_t count) noexcept
        {
            if (count == 0) {
                return 0;
            }

            if (b.empty()) {
                return 0;
            }

            const std::int64_t num_set{ count > b.size() ? b.size() : count };

            for (std::int64_t i = 0; i < num_set; ++i) {
                b.data()[i] = value;
            }
            return num_set;
        }

        template <typename T1, typename T2>
        inline constexpr std::int64_t set(Block<T1> b, const T2& value) noexcept
        {
            return set(b, value, b.size());
        }

        template <>
        class Block<void> {
        public:
            using Size_type = std::int64_t;
            using Type = void;
            using Pointer = void*;
            using Const_pointer = const void*;

            constexpr Block(const Block&) noexcept = default;
            constexpr Block& operator=(const Block&) noexcept = default;
            constexpr Block(Block&&) noexcept = default;
            constexpr Block& operator=(Block&&) noexcept = default;
            constexpr ~Block() noexcept = default;

            // Do not allow parially empty block
            constexpr Block(Size_type s = 0, Const_pointer p = nullptr, std::int64_t hint = std::numeric_limits<std::int64_t>::min()) noexcept
                : s_(p ? (s > 0 ? s : 0) : 0), p_(s > 0 ? const_cast<Pointer>(p) : nullptr), hint_(hint)
            {
            }

            [[nodiscard]] constexpr bool empty() const noexcept
            {
                return !s_ && !p_;
            }

            [[nodiscard]] constexpr Size_type size() const noexcept
            {
                return s_;
            }

            constexpr Pointer data() const noexcept
            {
                return p_;
            }

            [[nodiscard]] constexpr std::int64_t hint() const noexcept
            {
                return hint_;
            }

        private:
            Size_type s_{ 0 };
            Pointer p_{ nullptr };
            std::int64_t hint_{ std::numeric_limits<std::int64_t>::min() };
        };

        template <typename T1, typename T2>
            requires (std::is_same_v<void, T1> || std::is_same_v<void, T2>)
        [[nodiscard]] inline constexpr bool operator==(const Block<T1>& lhs, const Block<T2>& rhs) noexcept
        {
            constexpr const std::int64_t T1_size = MEMOC_SSIZEOF(std::conditional_t<std::is_same_v<void, T1>, std::uint8_t, T1>);
            constexpr const std::int64_t T2_size = MEMOC_SSIZEOF(std::conditional_t<std::is_same_v<void, T2>, std::uint8_t, T2>);

            if (lhs.size() * T1_size != rhs.size() * T2_size) {
                return false;
            }

            if (lhs.empty()) {
                return true;
            }

            const std::uint8_t* lhs_ptr{ reinterpret_cast<const std::uint8_t*>(lhs.data()) };
            const std::uint8_t* rhs_ptr{ reinterpret_cast<const std::uint8_t*>(rhs.data()) };

            for (std::int64_t i = 0; i < T1_size; ++i) {
                if (lhs_ptr[i] != rhs_ptr[i]) {
                    return false;
                }
            }

            return true;
        }

        template <typename T1, typename T2>
            requires (std::is_same_v<void, T1> || std::is_same_v<void, T2>)
        inline constexpr std::int64_t copy(const Block<T1>& src, Block<T2> dst, std::int64_t bytes) noexcept
        {
            if (bytes == 0) {
                return 0;
            }

            if (src.empty() || dst.empty()) {
                return 0;
            }

            constexpr const std::int64_t T1_size = MEMOC_SSIZEOF(std::conditional_t<std::is_same_v<void, T1>, std::uint8_t, T1>);
            constexpr const std::int64_t T2_size = MEMOC_SSIZEOF(std::conditional_t<std::is_same_v<void, T2>, std::uint8_t, T2>);

            const std::int64_t src_bytes_size = src.size() * T1_size;
            const std::int64_t dst_bytes_size = dst.size() * T2_size;

            const std::int64_t min_size{ src_bytes_size > dst_bytes_size ? dst_bytes_size : src_bytes_size };
            const std::int64_t num_copied{ bytes > min_size ? min_size : bytes };

            const std::uint8_t* src_ptr{ reinterpret_cast<const std::uint8_t*>(src.data()) };
            std::uint8_t* dst_ptr{ reinterpret_cast<std::uint8_t*>(dst.data()) };

            for (std::int64_t i = 0; i < num_copied; ++i) {
                dst_ptr[i] = src_ptr[i];
            }
            return num_copied;
        }

        template <typename T1, typename T2>
            requires (std::is_same_v<void, T1> || std::is_same_v<void, T2>)
        inline constexpr std::int64_t copy(const Block<T1>& src, Block<T2> dst) noexcept
        {
            constexpr const std::int64_t T1_size = MEMOC_SSIZEOF(std::conditional_t<std::is_same_v<void, T1>, std::uint8_t, T1>);
            return copy(src, dst, src.size() *  T1_size);
        }

        template <typename T>
        inline constexpr std::int64_t set(Block<void> b, const T& value, std::int64_t count) noexcept
        {
            if (count == 0) {
                return 0;
            }

            if (b.empty()) {
                return 0;
            }

            const std::int64_t block_size_by_type{ b.size() / MEMOC_SSIZEOF(T) };
            const std::int64_t num_set{ count > block_size_by_type ? block_size_by_type : count };

            T* ptr{ reinterpret_cast<T*>(b.data()) };
            for (std::int64_t i = 0; i < num_set; ++i) {
                ptr[i] = value;
            }
            return num_set;
        }

        template <typename T>
        inline constexpr std::int64_t set(Block<void> b, const T& value) noexcept
        {
            return set(b, value, b.size() / MEMOC_SSIZEOF(T));
        }
    }

    using details::Block2;

    using details::Block;

    using details::empty;
    using details::size;
    using details::data;
    using details::copy;
    using details::set;
}

#endif // MEMOC_BLOCKS_H