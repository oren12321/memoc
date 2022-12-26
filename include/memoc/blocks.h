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
        constexpr std::int64_t safe_64_unsigned_to_signed_cast() noexcept
        {
            static_assert(U <= std::numeric_limits<std::int64_t>::max(), "unsgined value is too large for casting to its unsgined version");
            return static_cast<std::int64_t>(U);
        }

        constexpr std::int64_t safe_64_unsigned_to_signed_cast(std::uint64_t u) noexcept
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

            void clear() noexcept
            {
                p_ = nullptr;
                s_ = 0;
            }

            bool empty() const noexcept
            {
                return p_ == nullptr && s_ == 0;
            }

            Size_type s() const noexcept
            {
                return s_;
            }

            Const_pointer p() const noexcept
            {
                return p_;
            }

            Pointer p() noexcept
            {
                return p_;
            }

        private:
            Size_type s_{ 0 };
            Pointer p_{ nullptr };
        };

        template <typename T1, typename T2>
        inline bool operator==(const Block<T1>& lhs, const Block<T2> rhs) noexcept
        {
            if (lhs.empty() && rhs.empty()) {
                return true;
            }

            if (lhs.s() != rhs.s()) {
                return false;
            }

            if (lhs.s() == 0) {
                return false;
            }

            bool still_equal{ true };
            for (std::int64_t i = 0; i < lhs.s() && still_equal; ++i) {
                still_equal &= (lhs.p()[i] == rhs.p()[i]);
            }
            return still_equal;
        }
    }

    using details::Block;
}

#endif // MEMOC_BLOCKS_H