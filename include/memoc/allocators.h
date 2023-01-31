#ifndef MEMOC_ALLOCATORS_H
#define MEMOC_ALLOCATORS_H

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <new>
#include <chrono>
#include <utility>
#include <type_traits>
#include <concepts>
#include <memory>

#include <erroc/errors.h>
#include <enumoc/enumoc.h>

#include <memoc/blocks.h>

ENUMOC_GENERATE(memoc, Allocator_error,
    invalid_size,
    out_of_memory,
    unknown);

namespace memoc {
    namespace details {
        template <class T>
        concept Allocator = 
            requires
        {
            std::is_default_constructible_v<T>;
            std::is_copy_constructible_v<T>;
            std::is_copy_assignable_v<T>;
            std::is_move_constructible_v<T>;
            std::is_move_assignable_v<T>;
            std::is_destructible_v<T>;
        }&&
            requires (T t, Block<void>::Size_type s, Block<void> b)
        {
            {t.allocate(s)} noexcept -> std::same_as<erroc::Expected<Block<void>, Allocator_error>>;
            {t.deallocate(std::ref(b))} noexcept -> std::same_as<void>;
            {t.owns(std::cref(b))} noexcept -> std::same_as<bool>;
        };

        template <Allocator Primary, Allocator Fallback>
        class Fallback_allocator final {
        public:
            //constexpr Fallback_allocator() = default;
            //constexpr Fallback_allocator(const Fallback_allocator& other) noexcept
            //    : primary_(other.primary_), fallback_(other.fallback_)
            //{
            //}
            //constexpr Fallback_allocator operator=(const Fallback_allocator& other) noexcept
            //{
            //    if (this == &other) {
            //        return *this;
            //    }
            //    primary_ = other.primary_;
            //    fallback_ = other.fallback_;
            //    return *this;
            //}
            //constexpr Fallback_allocator(Fallback_allocator&& other) noexcept
            //    : primary_(std::move(other.primary_)), fallback_(std::move(other.fallback_)) {}
            //constexpr Fallback_allocator& operator=(Fallback_allocator&& other) noexcept
            //{
            //    if (this == &other) {
            //        return *this;
            //    }
            //    primary_ = std::move(other.primary_);
            //    fallback_ = std::move(other.fallback_);
            //    return *this;
            //}
            //constexpr ~Fallback_allocator() = default;

            [[nodiscard]] constexpr erroc::Expected<Block<void>, Allocator_error> allocate(Block<void>::Size_type s) noexcept
            {
                if (erroc::Expected<Block<void>, Allocator_error> r = primary_.allocate(s)) {
                    return r;
                }
                return fallback_.allocate(s);
            }

            constexpr void deallocate(Block<void>& b) noexcept
            {
                if (primary_.owns(b)) {
                    return primary_.deallocate(b);
                }
                else if(fallback_.owns(b)) {
                    fallback_.deallocate(b);
                }
            }

            [[nodiscard]] constexpr bool owns(const Block<void>& b) const noexcept
            {
                return primary_.owns(b) || fallback_.owns(b);
            }

        private:
            Primary primary_;
            Fallback fallback_;
        };

        [[nodiscard]] constexpr std::int64_t encode(const char* str) noexcept {
            const char* p = str;
            std::uint64_t code = 0;
            while (!*p == '\0' && code < std::numeric_limits<std::int64_t>::max()) {
                std::uint64_t char_code = static_cast<std::uint64_t>(*p++);
                code |= char_code;
                code <<= (sizeof(*p) * 8);
            }
            return static_cast<std::int64_t>(code);
        }

        class Malloc_allocator final {
        public:
            [[nodiscard]] constexpr erroc::Expected<Block<void>, Allocator_error> allocate(Block<void>::Size_type s) noexcept
            {
                if (s < 0) {
                    return erroc::Unexpected(Allocator_error::invalid_size);
                }
                if (s == 0) {
                    return Block<void>();
                }
                Block<void> b(s, std::malloc(s), uuid_);
                if (b.empty()) {
                    return erroc::Unexpected(Allocator_error::unknown);
                }
                return b;
            }

            void deallocate(Block<void>& b) noexcept
            {
                std::free(b.data());
                b = Block<void>();
            }

            [[nodiscard]] constexpr bool owns(const Block<void>& b) const noexcept
            {
                return b.data() && b.hint() == uuid_;
            }

        private:
            constexpr static std::int64_t uuid_ = encode("095deb2c-f51a-4193-b177-d6d686087c72");
        };

        template <std::int64_t Stacks_count, Block<void>::Size_type Buffer_size>
        class Default_stacks_manager {
            static_assert(Stacks_count > 0);
            static_assert(Buffer_size > 1 && Buffer_size % 2 == 0);
        public:
            constexpr Default_stacks_manager() noexcept {
                if (!initialized_) {
                    for (std::int64_t i = 0; i < Stacks_count; ++i) {
                        ptrs_[i] = buffers_[i];
                    }
                    initialized_ = true;
                }
            }

            [[nodiscard]] constexpr void* stack_malloc(Block<void>::Size_type s) noexcept
            {
                for (std::int64_t i = 0; i < Stacks_count; ++i) {
                    if (Buffer_size - (ptrs_[i] - buffers_[i]) >= s) {
                        void* tmp = ptrs_[i];
                        ptrs_[i] += s;
                        return tmp;
                    }
                }
                return nullptr;
            }

            constexpr void stack_free(void* p, Block<void>::Size_type s) noexcept
            {
                for (std::int64_t i = 0; i < Stacks_count; ++i) {
                    if (p == ptrs_[i] - s) {
                        ptrs_[i] = reinterpret_cast<std::uint8_t*>(p);
                        break;
                    }
                }
            }

            [[nodiscard]] constexpr bool stack_owns(void* p) const noexcept
            {
                for (std::int64_t i = 0; i < Stacks_count; ++i) {
                    const std::uint8_t* lp = reinterpret_cast<const std::uint8_t*>(p);
                    if (lp >= buffers_[i] && lp < buffers_[i] + Buffer_size) {
                        return true;
                    }
                }
                return false;
            }

        private:
            inline static std::uint8_t buffers_[Stacks_count][Buffer_size];
            inline static std::uint8_t* ptrs_[Stacks_count];

            inline static bool initialized_{ false };
        };

        template <typename Stacks_manager = Default_stacks_manager<16, 128>>
        class Stack_allocator final {
        public:
            [[nodiscard]] constexpr erroc::Expected<Block<void>, Allocator_error> allocate(Block<void>::Size_type s) noexcept
            {
                if (s < 0) {
                    return erroc::Unexpected(Allocator_error::invalid_size);
                }
                if (s == 0) {
                    return Block<void>();
                }
                void* p = sm_.stack_malloc(align(s));
                if (!p) {
                    return erroc::Unexpected(Allocator_error::out_of_memory);
                }
                return Block<void>(s, p);
            }

            constexpr void deallocate(Block<void>& b) noexcept
            {
                sm_.stack_free(b.data(), b.size());
                b = {};
            }

            [[nodiscard]] constexpr bool owns(const Block<void>& b) const noexcept
            {
                return sm_.stack_owns(b.data());
            }

        private:
            constexpr Block<void>::Size_type align(Block<void>::Size_type s)
            {
                return s % 2 == 0 ? s : s + 1;
            }

            Stacks_manager sm_{};
        };

        template <
            Allocator Internal_allocator,
            Block<void>::Size_type Min_size, Block<void>::Size_type Max_size, std::int64_t Max_list_size>
            class Free_list_allocator final {
            static_assert(Min_size > 1 && Min_size % 2 == 0);
            static_assert(Max_size > 1 && Max_size % 2 == 0);
            static_assert(Max_list_size > 0);
            public:
                constexpr Free_list_allocator() = default;
                constexpr Free_list_allocator(const Free_list_allocator& other) noexcept
                    : internal_(other.internal_), root_(nullptr), list_size_(0) {}
                constexpr Free_list_allocator operator=(const Free_list_allocator& other) noexcept
                {
                    if (this == &other) {
                        return *this;
                    }

                    internal_ = other.internal_;
                    root_ = nullptr;
                    list_size_ = 0;
                    return *this;
                }
                constexpr Free_list_allocator(Free_list_allocator&& other) noexcept
                    : internal_(std::move(other.internal_)), root_(other.root_), list_size_(other.list_size_)
                {
                    other.root_ = nullptr;
                    other.list_size_ = 0;
                }
                constexpr Free_list_allocator& operator=(Free_list_allocator&& other) noexcept
                {
                    if (this == &other) {
                        return *this;
                    }

                    internal_ = std::move(other.internal_);
                    root_ = other.root_;
                    list_size_ = other.list_size_;
                    other.root_ = nullptr;
                    other.list_size_ = 0;
                    return *this;
                }
                // Responsible to release the saved memory blocks
                constexpr ~Free_list_allocator() noexcept
                {
                    for (std::int64_t i = 0; i < list_size_; ++i) {
                        Node* n = root_;
                        root_ = root_->next;
                        Block<void> b{ Max_size, n, n->hint };
                        internal_.deallocate(b);
                    }
                }

                [[nodiscard]] constexpr erroc::Expected<Block<void>, Allocator_error> allocate(Block<void>::Size_type s) noexcept
                {
                    if (s >= Min_size && s <= Max_size && list_size_ > 0 && root_) {
                        Block<void> b(s, root_, root_->hint);
                        root_ = root_->next;
                        --list_size_;
                        return b;
                    }
                    erroc::Expected<Block<void>, Allocator_error> r = internal_.allocate((s < Min_size || s > Max_size) ? s : Max_size);
                    if (!r) {
                        return r;
                    }
                    return Block<void>(s, r.value().data(), r.value().hint());
                }

                constexpr void deallocate(Block<void>& b) noexcept
                {
                    if (b.size() < Min_size || b.size() > Max_size || list_size_ > Max_list_size) {
                        Block<void> nb{ Max_size, b.data(), b.hint() };
                        b = Block<void>();
                        return internal_.deallocate(nb);
                    }
                    Node* node = reinterpret_cast<Node*>(b.data());
                    node->hint = b.hint();
                    node->next = root_;
                    root_ = node;
                    ++list_size_;
                    b = Block<void>();
                }

                [[nodiscard]] constexpr bool owns(const Block<void>& b) const noexcept
                {
                    return (b.size() >= Min_size && b.size() <= Max_size) || internal_.owns(b);
                }
            private:
                Internal_allocator internal_;

                struct Node {
                    std::int64_t hint{ std::numeric_limits<std::int64_t>::min() };
                    Node* next{ nullptr };
                };

                Node* root_{ nullptr };
                std::int64_t list_size_{ 0 };
        };

        template <typename T, Allocator Internal_allocator>
            requires (!std::is_reference_v<T>)
        class Stl_adapter_allocator {
        public:
            using value_type = T;

            constexpr Stl_adapter_allocator() = default;
            constexpr Stl_adapter_allocator(const Stl_adapter_allocator& other) noexcept
                : internal_(other.internal_) {}
            constexpr Stl_adapter_allocator operator=(const Stl_adapter_allocator& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }
                internal_ = other.internal_;
                return *this;
            }
            constexpr Stl_adapter_allocator(Stl_adapter_allocator&& other) noexcept
                : internal_(std::move(other.internal_)) {}
            constexpr Stl_adapter_allocator& operator=(Stl_adapter_allocator&& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }
                internal_ = std::move(other.internal_);
                return *this;
            }
            constexpr ~Stl_adapter_allocator() = default;

            template <typename U>
                requires (!std::is_reference_v<U>)
            constexpr Stl_adapter_allocator(const Stl_adapter_allocator<U, Internal_allocator>&) noexcept {}

            [[nodiscard]] constexpr T* allocate(std::size_t n)
            {
                erroc::Expected<Block<void>, Allocator_error> r = internal_.allocate(n * MEMOC_SSIZEOF(T));
                if (!r) {
                    throw std::bad_alloc{};
                }
                return reinterpret_cast<T*>(r.value().data());
            }

            constexpr void deallocate(T* p, std::size_t n) noexcept
            {
                Block<void> b = { safe_64_unsigned_to_signed_cast(n) * MEMOC_SSIZEOF(T), reinterpret_cast<void*>(p) };
                internal_.deallocate(b);
            }

        private:
            Internal_allocator internal_;
        };

        template <Allocator Internal_allocator, std::int64_t Number_of_records>
        class Stats_allocator final {
        public:
            struct Record {
                void* record_address{ nullptr };
                void* request_address{ nullptr };
                Block<void>::Size_type amount{ 0 };
                std::chrono::time_point<std::chrono::system_clock> time;
                Record* next{ nullptr };
            };

            constexpr Stats_allocator() = default;
            constexpr Stats_allocator(const Stats_allocator& other) noexcept
                : internal_(other.internal_)
            {
                for (Record* r = other.root_; r != nullptr; r = r->next) {
                    add_record(r->request_address, r->amount - MEMOC_SSIZEOF(Record), r->time);
                }
            }
            constexpr Stats_allocator operator=(const Stats_allocator& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }
                internal_ = other.internal_;
                for (Record* r = other.root_; r != nullptr; r = r->next) {
                    add_record(r->request_address, r->amount - MEMOC_SSIZEOF(Record), r->time);
                }
                return *this;
            }
            constexpr Stats_allocator(Stats_allocator&& other) noexcept
                : internal_(std::move(other.internal_)), number_of_records_(other.number_of_records_), total_allocated_(other.total_allocated_), root_(other.root_), tail_(other.tail_)
            {
                other.number_of_records_ = other.total_allocated_ = 0;
                other.root_ = other.tail_ = nullptr;
            }
            constexpr Stats_allocator& operator=(Stats_allocator&& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }
                internal_ = std::move(other.internal_);
                number_of_records_ = other.number_of_records_;
                total_allocated_ = other.total_allocated_;
                root_ = other.root_;
                tail_ = other.tail_;
                other.number_of_records_ = other.total_allocated_ = 0;
                other.root_ = other.tail_ = nullptr;
                return *this;
            }
            constexpr ~Stats_allocator() noexcept
            {
                Record* c = root_;
                while (c) {
                    Record* n = c->next;
                    Block<void> b{ MEMOC_SSIZEOF(Record), c->record_address };
                    internal_.deallocate(b);
                    c = n;
                }
            }

            [[nodiscard]] constexpr erroc::Expected<Block<void>, Allocator_error> allocate(Block<void>::Size_type s) noexcept
            {
                erroc::Expected<Block<void>, Allocator_error> r = internal_.allocate(s);
                if (!r) {
                    return r;
                }
                Block<void> b(r.value());
                if (!b.empty()) {
                    add_record(b.data(), b.size());
                }
                return b;
            }

            void constexpr deallocate(Block<void>& b) noexcept
            {
                Block<void> bc{ b };
                internal_.deallocate(b);
                if (b.empty()) {
                    add_record(bc.data(), -bc.size());
                }
            }

            [[nodiscard]] constexpr bool owns(const Block<void>& b) const noexcept
            {
                return internal_.owns(b);
            }

            constexpr const Record* stats_list() const noexcept {
                return root_;
            }

            constexpr std::int64_t stats_list_size() const noexcept {
                return number_of_records_;
            }

            constexpr Block<void>::Size_type total_allocated() const noexcept {
                return total_allocated_;
            }

        private:
            constexpr void add_record(void* p, Block<void>::Size_type a, std::chrono::time_point<std::chrono::system_clock> time = std::chrono::system_clock::now()) {
                if (number_of_records_ >= Number_of_records) {
                    tail_->next = root_;
                    root_ = root_->next;
                    tail_ = tail_->next;
                    tail_->next = nullptr;
                    tail_->request_address = p;
                    tail_->amount = MEMOC_SSIZEOF(Record) + a;
                    tail_->time = time;

                    total_allocated_ += tail_->amount;

                    return;
                }

                erroc::Expected<Block<void>, Allocator_error> r = internal_.allocate(MEMOC_SSIZEOF(Record));
                if (!r) {
                    return;
                }
                Block<void> b1(r.value());
                if (b1.empty()) {
                    return;
                }

                if (!root_) {
                    root_ = reinterpret_cast<Record*>(b1.data());
                    tail_ = root_;
                }
                else {
                    tail_->next = reinterpret_cast<Record*>(b1.data());
                    tail_ = tail_->next;
                }
                tail_->record_address = b1.data();
                tail_->request_address = p;
                tail_->amount = b1.size() + a;
                tail_->time = time;
                tail_->next = nullptr;

                total_allocated_ += tail_->amount;

                ++number_of_records_;
            }

            Internal_allocator internal_;

            std::int64_t number_of_records_{ 0 };
            Block<void>::Size_type total_allocated_{ 0 };
            Record* root_{ nullptr };
            Record* tail_{ nullptr };
        };

        template <Allocator Internal_allocator, std::int64_t id = -1>
        class Shared_allocator final {
        public:
            [[nodiscard]] constexpr erroc::Expected<Block<void>, Allocator_error> allocate(Block<void>::Size_type s) noexcept
            {
                return allocator_.allocate(s);
            }

            constexpr void deallocate(Block<void>& b) noexcept
            {
                allocator_.deallocate(b);
            }

            [[nodiscard]] constexpr bool owns(const Block<void>& b) const noexcept
            {
                return allocator_.owns(b);
            }
        private:
            inline static Internal_allocator allocator_{};
        };

        // Allocators API

        template <Allocator T>
        [[nodiscard]] inline constexpr T create() noexcept
        {
            return T();
        }

        template <Allocator T>
        [[nodiscard]] inline constexpr erroc::Expected<Block<void>, Allocator_error> allocate(T& allocator, Block<void>::Size_type size) noexcept
        {
            return allocator.allocate(size);
        }

        template <Allocator T>
        inline constexpr void deallocate(T& allocator, Block<void>& block) noexcept
        {
            allocator.deallocate(block);
        }

        template <Allocator T>
        [[nodiscard]] inline constexpr bool owns(const T& allocator, const Block<void>& block) noexcept
        {
            return allocator.owns(block);
        }
    }

    using details::Allocator;
    using details::Fallback_allocator;
    using details::Free_list_allocator;
    using details::Malloc_allocator;
    using details::Malloc_allocator;
    using details::Shared_allocator;
    using details::Stack_allocator;
    using details::Stats_allocator;
    using details::Stl_adapter_allocator;
    using details::create;
    using details::allocate;
    using details::deallocate;
    using details::owns;
}

#endif // MEMOC_ALLOCATORS_H

