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

#include <memoc/blocks.h>

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
            requires (T t, std::size_t s, Block b)
        {
            {t.allocate(s)} noexcept -> std::same_as<decltype(b)>;
            {t.deallocate(&b)} noexcept -> std::same_as<void>;
            {t.owns(b)} noexcept -> std::same_as<bool>;
        };

        template <Allocator Primary, Allocator Fallback>
        class Fallback_allocator
            : private Primary
            , private Fallback {
        public:

            Fallback_allocator() = default;
            Fallback_allocator(const Fallback_allocator& other) noexcept
                : Primary(other), Fallback(other) {}
            Fallback_allocator operator=(const Fallback_allocator& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }
                Primary::operator=(other);
                Fallback::operator=(other);
                return *this;
            }
            Fallback_allocator(Fallback_allocator&& other) noexcept
                : Primary(std::move(other)), Fallback(std::move(other)) {}
            Fallback_allocator& operator=(Fallback_allocator&& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }
                Primary::operator=(std::move(other));
                Fallback::operator=(std::move(other));
                return *this;
            }
            virtual ~Fallback_allocator() = default;

            [[nodiscard]] Block allocate(Block::Size_type s) noexcept
            {
                Block b = Primary::allocate(s);
                if (b.empty()) {
                    b = Fallback::allocate(s);
                }
                return b;
            }

            void deallocate(Block* b) noexcept
            {
                if (Primary::owns(*b)) {
                    return Primary::deallocate(b);
                }
                Fallback::deallocate(b);
            }

            [[nodiscard]] bool owns(Block b) const noexcept
            {
                return Primary::owns(b) || Fallback::owns(b);
            }
        };

        class Malloc_allocator {
        public:
            [[nodiscard]] Block allocate(Block::Size_type s) noexcept
            {
                if (s == 0) {
                    return { s, nullptr };
                }
                return { s, std::malloc(s) };
            }

            void deallocate(Block* b) noexcept
            {
                std::free(b->p());
                b->clear();
            }

            [[nodiscard]] bool owns(Block b) const noexcept
            {
                return b.p();
            }
        };

        template <std::size_t Size>
        class Stack_allocator {
            static_assert(Size > 1 && Size % 2 == 0);
        public:
            Stack_allocator() = default;
            Stack_allocator(const Stack_allocator& other) noexcept
                : p_(d_) {}
            Stack_allocator operator=(const Stack_allocator& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }

                p_ = d_;
                return *this;
            }
            Stack_allocator(Stack_allocator&& other) noexcept
                : p_(d_)
            {
                other.p_ = nullptr;
            }
            Stack_allocator& operator=(Stack_allocator&& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }

                p_ = d_;
                other.p_ = nullptr;
                return *this;
            }
            virtual ~Stack_allocator() = default;

            [[nodiscard]] Block allocate(Block::Size_type s) noexcept
            {
                auto s1 = align(s);
                if (p_ + s1 > d_ + Size || !p_ || s == 0) {
                    return { 0, nullptr };
                }
                Block b = { s, p_ };
                p_ += s1;
                return b;
            }

            void deallocate(Block* b) noexcept
            {
                if (b->p() == p_ - align(b->s())) {
                    p_ = reinterpret_cast<std::uint8_t*>(b->p());
                }
                b->clear();
            }

            [[nodiscard]] bool owns(Block b) const noexcept
            {
                return b.p() >= d_ && b.p() < d_ + Size;
            }

        private:
            std::size_t align(std::size_t s)
            {
                return s % 2 == 0 ? s : s + 1;
            }

            std::uint8_t d_[Size] = { 0 };
            std::uint8_t* p_{ d_ };
        };

        template <
            Allocator Internal_allocator,
            std::size_t Min_size, std::size_t Max_size, std::size_t Max_list_size>
            class Free_list_allocator
            : private Internal_allocator {
            static_assert(Min_size > 1 && Min_size % 2 == 0);
            static_assert(Max_size > 1 && Max_size % 2 == 0);
            static_assert(Max_list_size > 0);
            public:
                Free_list_allocator() = default;
                Free_list_allocator(const Free_list_allocator& other) noexcept
                    : Internal_allocator(other), root_(nullptr), list_size_(0) {}
                Free_list_allocator operator=(const Free_list_allocator& other) noexcept
                {
                    if (this == &other) {
                        return *this;
                    }

                    Internal_allocator::operator=(other);
                    root_ = nullptr;
                    list_size_ = 0;
                    return *this;
                }
                Free_list_allocator(Free_list_allocator&& other) noexcept
                    : Internal_allocator(std::move(other)), root_(other.root_), list_size_(other.list_size_)
                {
                    other.root_ = nullptr;
                    other.list_size_ = 0;
                }
                Free_list_allocator& operator=(Free_list_allocator&& other) noexcept
                {
                    if (this == &other) {
                        return *this;
                    }

                    Internal_allocator::operator=(std::move(other));
                    root_ = other.root_;
                    list_size_ = other.list_size_;
                    other.root_ = nullptr;
                    other.list_size_ = 0;
                    return *this;
                }
                // Responsible to release the saved memory blocks
                virtual ~Free_list_allocator() noexcept
                {
                    for (std::size_t i = 0; i < list_size_; ++i) {
                        Node* n = root_;
                        root_ = root_->next;
                        Block b{ Max_size, n };
                        Internal_allocator::deallocate(&b);
                    }
                }

                [[nodiscard]] Block allocate(Block::Size_type s) noexcept
                {
                    if (s >= Min_size && s <= Max_size && list_size_ > 0) {
                        Block b = { s, root_ };
                        root_ = root_->next;
                        --list_size_;
                        return b;
                    }
                    Block b = { s, Internal_allocator::allocate((s < Min_size || s > Max_size) ? s : Max_size).p() };
                    return b;
                }

                void deallocate(Block* b) noexcept
                {
                    if (b->s() < Min_size || b->s() > Max_size || list_size_ > Max_list_size) {
                        Block nb{ Max_size, b->p() };
                        b->clear();
                        return Internal_allocator::deallocate(&nb);
                    }
                    auto node = reinterpret_cast<Node*>(b->p());
                    node->next = root_;
                    root_ = node;
                    ++list_size_;
                    b->clear();
                }

                [[nodiscard]] bool owns(Block b) const noexcept
                {
                    return (b.s() >= Min_size && b.s() <= Max_size) || Internal_allocator::owns(b);
                }
            private:
                struct Node {
                    Node* next{ nullptr };
                };

                Node* root_{ nullptr };
                std::size_t list_size_{ 0 };
        };

        template <typename T, Allocator Internal_allocator>
            requires (!std::is_reference_v<T>)
        class Stl_adapter_allocator
            : private Internal_allocator {
        public:
            using value_type = T;

            Stl_adapter_allocator() = default;
            Stl_adapter_allocator(const Stl_adapter_allocator& other) noexcept
                : Internal_allocator(other) {}
            Stl_adapter_allocator operator=(const Stl_adapter_allocator& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }
                Internal_allocator::operator=(other);
                return *this;
            }
            Stl_adapter_allocator(Stl_adapter_allocator&& other) noexcept
                : Internal_allocator(std::move(other)) {}
            Stl_adapter_allocator& operator=(Stl_adapter_allocator&& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }
                Internal_allocator::operator=(std::move(other));
                return *this;
            }
            virtual ~Stl_adapter_allocator() = default;

            template <typename U>
                requires (!std::is_reference_v<U>)
            constexpr Stl_adapter_allocator(const Stl_adapter_allocator<U, Internal_allocator>&) noexcept {}

            [[nodiscard]] T* allocate(std::size_t n)
            {
                Block b = Internal_allocator::allocate(n * sizeof(T));
                if (b.empty()) {
                    throw std::bad_alloc{};
                }
                return reinterpret_cast<T*>(b.p());
            }

            void deallocate(T* p, std::size_t n) noexcept
            {
                Block b = { n * sizeof(T), reinterpret_cast<void*>(p) };
                Internal_allocator::deallocate(&b);
            }
        };

        template <Allocator Internal_allocator, std::size_t Number_of_records>
        class Stats_allocator
            : private Internal_allocator {
        public:
            struct Record {
                void* record_address{ nullptr };
                void* request_address{ nullptr };
                std::int64_t amount{ 0 };
                std::chrono::time_point<std::chrono::system_clock> time;
                Record* next{ nullptr };
            };

            Stats_allocator() = default;
            Stats_allocator(const Stats_allocator& other) noexcept
                : Internal_allocator(other)
            {
                for (Record* r = other.root_; r != nullptr; r = r->next) {
                    add_record(r->request_address, r->amount - sizeof(Record), r->time);
                }
            }
            Stats_allocator operator=(const Stats_allocator& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }
                Internal_allocator::operator=(other);
                for (Record* r = other.root_; r != nullptr; r = r->next) {
                    add_record(r->request_address, r->amount - sizeof(Record), r->time);
                }
                return *this;
            }
            Stats_allocator(Stats_allocator&& other) noexcept
                : Internal_allocator(std::move(other)), number_of_records_(other.number_of_records_), total_allocated_(other.total_allocated_), root_(other.root_), tail_(other.tail_)
            {
                other.number_of_records_ = other.total_allocated_ = 0;
                other.root_ = other.tail_ = nullptr;
            }
            Stats_allocator& operator=(Stats_allocator&& other) noexcept
            {
                if (this == &other) {
                    return *this;
                }
                Internal_allocator::operator=(std::move(other));
                number_of_records_ = other.number_of_records_;
                total_allocated_ = other.total_allocated_;
                root_ = other.root_;
                tail_ = other.tail_;
                other.number_of_records_ = other.total_allocated_ = 0;
                other.root_ = other.tail_ = nullptr;
                return *this;
            }
            virtual ~Stats_allocator() noexcept
            {
                Record* c = root_;
                while (c) {
                    Record* n = c->next;
                    Block b{ sizeof(Record), c->record_address };
                    Internal_allocator::deallocate(&b);
                    c = n;
                }
            }

            [[nodiscard]] Block allocate(Block::Size_type s) noexcept
            {
                Block b = Internal_allocator::allocate(s);
                if (!b.empty()) {
                    add_record(b.p(), static_cast<std::int64_t>(b.s()));
                }
                return b;
            }

            void deallocate(Block* b) noexcept
            {
                Block bc{ *b };
                Internal_allocator::deallocate(b);
                if (b->empty()) {
                    add_record(bc.p(), -static_cast<std::int64_t>(bc.s()));
                }
            }

            [[nodiscard]] bool owns(Block b) const noexcept
            {
                return Internal_allocator::owns(b);
            }

            const Record* stats_list() const noexcept {
                return root_;
            }

            std::size_t stats_list_size() const noexcept {
                return number_of_records_;
            }

            std::int64_t total_allocated() const noexcept {
                return total_allocated_;
            }

        private:
            void add_record(void* p, std::int64_t a, std::chrono::time_point<std::chrono::system_clock> time = std::chrono::system_clock::now()) {
                if (number_of_records_ >= Number_of_records) {
                    tail_->next = root_;
                    root_ = root_->next;
                    tail_ = tail_->next;
                    tail_->next = nullptr;
                    tail_->request_address = p;
                    tail_->amount = static_cast<std::int64_t>(sizeof(Record)) + a;
                    tail_->time = time;

                    total_allocated_ += tail_->amount;

                    return;
                }

                Block b1 = Internal_allocator::allocate(sizeof(Record));
                if (b1.empty()) {
                    return;
                }

                if (!root_) {
                    root_ = reinterpret_cast<Record*>(b1.p());
                    tail_ = root_;
                }
                else {
                    tail_->next = reinterpret_cast<Record*>(b1.p());
                    tail_ = tail_->next;
                }
                tail_->record_address = b1.p();
                tail_->request_address = p;
                tail_->amount = static_cast<std::int64_t>(b1.s()) + a;
                tail_->time = time;
                tail_->next = nullptr;

                total_allocated_ += tail_->amount;

                ++number_of_records_;
            }

            std::size_t number_of_records_{ 0 };
            std::int64_t total_allocated_{ 0 };
            Record* root_{ nullptr };
            Record* tail_{ nullptr };
        };

        template <Allocator Internal_allocator, int id = -1>
        class Shared_allocator {
        public:
            [[nodiscard]] Block allocate(Block::Size_type s) noexcept
            {
                return allocator_.allocate(s);
            }

            void deallocate(Block* b) noexcept
            {
                allocator_.deallocate(b);
            }

            [[nodiscard]] bool owns(Block b) const noexcept
            {
                return allocator_.owns(b);
            }
        private:
            inline static Internal_allocator allocator_{};
        };
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
}

#endif // MEMOC_ALLOCATORS_H

