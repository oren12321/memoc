#ifndef MEMOC_BLOCKS_H
#define MEMOC_BLOCKS_H

#include <cstdint>
#include <limits>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace memoc {
    namespace details {
        template <std::uint64_t U>
        constexpr std::int64_t safe_64_unsigned_to_signed_cast()
        {
            static_assert(U <= std::numeric_limits<std::int64_t>::max(), "unsgined value is too large for casting to its unsgined version");
            return static_cast<std::int64_t>(U);
        }
    }
}

#define MEMOC_SSIZEOF(expression) memoc::details::safe_64_unsigned_to_signed_cast<sizeof(expression)>()

namespace memoc {
    namespace details {
        template <typename T>
            requires (!std::is_reference_v<T>)
        class Typed_block {
        public:
            using Size_type = std::size_t;
            using Pointer = T*;
            using Const_pointer = const T*;

            Typed_block(const Typed_block&) noexcept = default;
            Typed_block& operator=(const Typed_block&) noexcept = default;
            Typed_block(Typed_block&&) noexcept = default;
            Typed_block& operator=(Typed_block&&) noexcept = default;
            virtual ~Typed_block() noexcept = default;

            // Do not allow parially empty block
            Typed_block(Size_type s = 0, Const_pointer p = nullptr) noexcept
                : s_(p ? s : 0), p_(s ? const_cast<Pointer>(p) : nullptr)
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
        inline bool operator==(const Typed_block<T1>& lhs, const Typed_block<T2> rhs) noexcept
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
            for (std::size_t i = 0; i < lhs.s() && still_equal; ++i) {
                still_equal &= (lhs.p()[i] == rhs.p()[i]);
            }
            return still_equal;
        }
    }

    using details::Typed_block;
    using Block = Typed_block<void>;
}

#endif // MEMOC_BLOCKS_H