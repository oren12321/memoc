#ifndef MEMOC_BLOCKS_H
#define MEMOC_BLOCKS_H

#include <cstddef>
#include <type_traits>
#include <utility>

namespace memoc {
    namespace details {
        template <typename T>
            requires (!std::is_reference_v<T>)
        struct Typed_block {
            using Size_type = std::size_t;
            using Pointer = T*;
            using Const_pointer = const T*;

            Size_type s{ 0 };
            Pointer p{ nullptr };

            Typed_block() noexcept = default;
            Typed_block(const Typed_block&) noexcept = default;
            Typed_block& operator=(const Typed_block&) noexcept = default;
            Typed_block(Typed_block&&) noexcept = default;
            Typed_block& operator=(Typed_block&&) noexcept = default;
            virtual ~Typed_block() noexcept = default;

            Typed_block(Size_type ns, Const_pointer np) noexcept
                : s(ns), p(const_cast<Pointer>(np)) {}


            void clear() noexcept
            {
                p = nullptr;
                s = 0;
            }

            bool empty() const noexcept
            {
                return p == nullptr && s == 0;
            }
        };

        template <typename T1, typename T2>
        inline bool operator==(const Typed_block<T1>& lhs, const Typed_block<T2> rhs) noexcept
        {
            if (lhs.empty() && rhs.empty()) {
                return true;
            }

            if (lhs.s != rhs.s) {
                return false;
            }

            if (lhs.s == 0) {
                return false;
            }

            bool still_equal{ true };
            for (std::size_t i = 0; i < lhs.s && still_equal; ++i) {
                still_equal &= (lhs.p[i] == rhs.p[i]);
            }
            return still_equal;
        }
    }

    using details::Typed_block;
    using Block = Typed_block<void>;
}

#endif // MEMOC_BLOCKS_H