#ifndef MEMOC_BLOCKS_H
#define MEMOC_BLOCKS_H

#include <cstddef>
#include <type_traits>
#include <utility>

namespace memoc::blocks {
    namespace details {
        template <typename T>
        concept Base_type = !std::is_pointer_v<T> && !std::is_reference_v<T>;

        template <Base_type T>
        struct Typed_block {
            using Pointer = T*;
            using Size_type = std::size_t;

            Pointer p{ nullptr };
            Size_type s{ 0 };

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
    }

    using details::Typed_block;
    using Block = Typed_block<void>;
}

#endif // MEMOC_BLOCKS_H