#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <array>
#include <vector>
#include <chrono>
#include <utility>
#include <limits>

#include <memoc/allocators.h>
#include <memoc/blocks.h>

// Malloc_allocator tests

class Malloc_allocator_test : public ::testing::Test {
protected:
    using Allocator = memoc::Malloc_allocator;
    Allocator allocator_{};
};

TEST_F(Malloc_allocator_test, not_owns_an_empty_block)
{
    using namespace memoc;

    EXPECT_FALSE(allocator_.owns(Block<void>{}));
}

TEST_F(Malloc_allocator_test, allocates_and_deallocates_memory_successfully)
{
    using namespace memoc;

    const Block<void>::Size_type s{ 1 };

    Block<void> b = allocator_.allocate(s).value();
    EXPECT_NE(nullptr, b.data());
    EXPECT_EQ(1, b.size());

    EXPECT_TRUE(allocator_.owns(b));

    allocator_.deallocate(b);
    EXPECT_TRUE(b.empty());

    EXPECT_TRUE(allocator_.allocate(0).value().empty());
}

TEST_F(Malloc_allocator_test, failed_to_allocate_if_invalid_size)
{
    using namespace memoc;

    EXPECT_EQ(Allocator_error::invalid_size, allocator_.allocate(-1).error());

    // Comment in order to prevent address-sanitizer error in Linux.
    //EXPECT_EQ(Allocator_error::unknown, allocator_.allocate(std::numeric_limits<Block<void>::Size_type>::max()).error());
}

// Stack_allocator tests

class Stack_allocator_test : public ::testing::Test {
protected:
    static constexpr std::int64_t count_ = 1;
    static constexpr memoc::Block<void>::Size_type size_ = 16;

    using Allocator = memoc::Stack_allocator<memoc::details::Default_global_stack_memory<count_, size_>>;
    Allocator allocator_{};
};

TEST_F(Stack_allocator_test, not_owns_an_empty_block)
{
    using namespace memoc;

    EXPECT_FALSE(allocator_.owns(Block<void>{}));
}

TEST_F(Stack_allocator_test, allocates_and_deallocates_an_arbitrary_in_range_memory_successfully)
{
    using namespace memoc;

    const Block<void>::Size_type size_in_range{ size_ / 2 };

    Block<void> b = allocator_.allocate(size_in_range).value();
    EXPECT_NE(nullptr, b.data());
    EXPECT_EQ(size_in_range, b.size());

    EXPECT_TRUE(allocator_.owns(b));

    allocator_.deallocate(b);
    EXPECT_TRUE(b.empty());
}

TEST_F(Stack_allocator_test, allocates_and_deallocates_all_available_memory_successfully)
{
    using namespace memoc;

    Block<void> b = allocator_.allocate(size_).value();
    EXPECT_NE(nullptr, b.data());
    EXPECT_EQ(size_, b.size());

    EXPECT_TRUE(allocator_.owns(b));

    allocator_.deallocate(b);
    EXPECT_TRUE(b.empty());
}

TEST_F(Stack_allocator_test, reuses_the_same_memory_if_deallocating_between_two_allocations)
{
    using namespace memoc;

    const Block<void>::Size_type size_in_range{ size_ / 2 };

    Block<void> b1 = allocator_.allocate(size_in_range).value();
    EXPECT_NE(nullptr, b1.data());
    EXPECT_EQ(size_in_range, b1.size());
    EXPECT_TRUE(allocator_.owns(b1));

    Block<void> b1_copy{ b1 };

    allocator_.deallocate(b1);
    EXPECT_TRUE(b1.empty());

    Block<void> b2 = allocator_.allocate(size_in_range).value();
    EXPECT_NE(nullptr, b2.data());
    EXPECT_EQ(size_in_range, b2.size());
    EXPECT_TRUE(allocator_.owns(b2));
    EXPECT_EQ(b1_copy.data(), b2.data());
    EXPECT_EQ(b1_copy.size(), b2.size());

    allocator_.deallocate(b2);
    EXPECT_TRUE(b2.empty());
}

TEST_F(Stack_allocator_test, fails_to_allocate_memory_bigger_than_memory_size)
{
    using namespace memoc;

    EXPECT_EQ(Allocator_error::out_of_memory, allocator_.allocate(size_ + 1).error());
}

TEST_F(Stack_allocator_test, DISABLED_is_copyable)
{
    using namespace memoc;

    Allocator copy1{ allocator_ };

    Block<void> b1 = allocator_.allocate(size_).value();
    Block<void> b2 = copy1.allocate(size_).value();

    EXPECT_FALSE(b1.empty());
    EXPECT_FALSE(b2.empty());
    EXPECT_EQ(size_, b1.size());
    EXPECT_EQ(size_, b2.size());
    EXPECT_NE(b1.data(), b2.data());

    Allocator copy2{};
    copy2 = allocator_;

    Block<void> b3 = copy2.allocate(size_).value();

    EXPECT_FALSE(b1.empty());
    EXPECT_FALSE(b3.empty());
    EXPECT_EQ(size_, b1.size());
    EXPECT_EQ(size_, b3.size());
    EXPECT_NE(b1.data(), b3.data());
}

TEST_F(Stack_allocator_test, DISABLED_is_movealbe)
{
    using namespace memoc;

    Allocator moved1{ std::move(allocator_) };

    //Block<void> b1 = allocator_.allocate(size_).value();
    EXPECT_EQ(Allocator_error::unknown, allocator_.allocate(size_).error());

    Block<void> b2 = moved1.allocate(size_).value();

    //EXPECT_TRUE(b1.empty());
    EXPECT_FALSE(b2.empty());
    EXPECT_EQ(size_, b2.size());
    EXPECT_NE(nullptr, b2.data());

    Allocator moved2{};
    moved2 = std::move(moved1);

    //Block<void> b3 = moved1.allocate(size_).value();
    EXPECT_EQ(Allocator_error::unknown, moved1.allocate(size_).error());

    Block<void> b4 = moved2.allocate(size_).value();

    //EXPECT_TRUE(b3.empty());
    EXPECT_FALSE(b4.empty());
    EXPECT_EQ(size_, b4.size());
    EXPECT_NE(nullptr, b4.data());
}

// Free_list_allocator tests

class Free_list_allocator_test : public ::testing::Test {
protected:
    static constexpr memoc::Block<void>::Size_type min_size_ = 16;
    static constexpr memoc::Block<void>::Size_type max_size_ = 32;
    static constexpr std::int64_t max_list_size_ = 2;
    using Parent = memoc::Malloc_allocator;

    using Allocator = memoc::Free_list_allocator<Parent, min_size_, max_size_, max_list_size_>;
    Allocator allocator_{};
};

TEST_F(Free_list_allocator_test, not_owns_an_empty_block)
{
    using namespace memoc;

    EXPECT_FALSE(allocator_.owns(Block<void>{}));
}

TEST_F(Free_list_allocator_test, allocates_and_deallocates_an_arbitrary_not_in_range_memory_successfully_using_parent_allocator)
{
    using namespace memoc;

    const Block<void>::Size_type size_out_of_range{ max_size_ + 1 };

    Block<void> b = allocator_.allocate(size_out_of_range).value();
    EXPECT_NE(nullptr, b.data());
    EXPECT_EQ(size_out_of_range, b.size());

    EXPECT_TRUE(allocator_.owns(b));

    allocator_.deallocate(b);
    EXPECT_TRUE(b.empty());
}

TEST_F(Free_list_allocator_test, reuses_the_same_memory_if_deallocating_in_memory_range)
{
    using namespace memoc;

    const Block<void>::Size_type size_in_range{ min_size_ + (max_size_ - min_size_) / 2 };

    std::array<Block<void>, max_list_size_> saved_blocks{};

    for (std::int64_t i = 0; i < max_list_size_; ++i)
    {
        Block<void> b = allocator_.allocate(size_in_range).value();
        EXPECT_NE(nullptr, b.data());
        EXPECT_EQ(size_in_range, b.size());
        EXPECT_TRUE(allocator_.owns(b));

        saved_blocks[i] = b;
    }

    for (std::array<Block<void>, max_list_size_>::const_reverse_iterator it = saved_blocks.rbegin(); it != saved_blocks.rend(); ++it)
    {
        Block<void> b_copy{ *it };
        allocator_.deallocate(b_copy);
        EXPECT_TRUE(b_copy.empty());
    }

    for (const auto& saved_block : saved_blocks)
    {
        Block<void> b = allocator_.allocate(size_in_range).value();
        EXPECT_NE(nullptr, b.data());
        EXPECT_EQ(size_in_range, b.size());
        EXPECT_TRUE(allocator_.owns(b));
        EXPECT_EQ(saved_block.data(), b.data());
        EXPECT_EQ(saved_block.size(), b.size());
    }

    // Test cleanup since free list does not actually release memory
    Parent p{};
    for (auto& saved_block : saved_blocks)
    {
        p.deallocate(saved_block);
        EXPECT_TRUE(saved_block.empty());
    }
}

TEST_F(Free_list_allocator_test, is_copyable)
{
    using namespace memoc;

    const Block<void>::Size_type size_in_range{ min_size_ + (max_size_ - min_size_) / 2 };

    Allocator copy1{ allocator_ };

    Block<void> b1 = allocator_.allocate(size_in_range).value();
    Block<void> b2 = copy1.allocate(size_in_range).value();

    EXPECT_FALSE(b1.empty());
    EXPECT_FALSE(b2.empty());
    EXPECT_EQ(size_in_range, b1.size());
    EXPECT_EQ(size_in_range, b2.size());
    EXPECT_NE(b1.data(), b2.data());

    Block<void> b1_copy = b1;
    allocator_.deallocate(b1);
    Block<void> b3 = allocator_.allocate(size_in_range).value();
    Block<void> b2_copy = b2;
    copy1.deallocate(b2);
    Block<void> b4 = copy1.allocate(size_in_range).value();

    EXPECT_EQ(b1_copy.size(), b3.size());
    EXPECT_EQ(b1_copy.data(), b3.data());
    EXPECT_EQ(b2_copy.size(), b4.size());
    EXPECT_EQ(b2_copy.data(), b4.data());

    allocator_.deallocate(b3);
    copy1.deallocate(b4);
    EXPECT_TRUE(b3.empty());
    EXPECT_TRUE(b4.empty());

    Allocator copy2{};
    copy2 = allocator_;

    Block<void> b5 = allocator_.allocate(size_in_range).value();
    Block<void> b6 = copy2.allocate(size_in_range).value();

    EXPECT_FALSE(b5.empty());
    EXPECT_FALSE(b6.empty());
    EXPECT_EQ(size_in_range, b5.size());
    EXPECT_EQ(size_in_range, b6.size());
    EXPECT_NE(b5.data(), b6.data());

    Block<void> b5_copy = b5;
    allocator_.deallocate(b5);
    Block<void> b7 = allocator_.allocate(size_in_range).value();
    Block<void> b6_copy = b6;
    copy2.deallocate(b6);
    Block<void> b8 = copy2.allocate(size_in_range).value();

    EXPECT_EQ(b5_copy.size(), b7.size());
    EXPECT_EQ(b5_copy.data(), b7.data());
    EXPECT_EQ(b6_copy.size(), b8.size());
    EXPECT_EQ(b6_copy.data(), b8.data());

    allocator_.deallocate(b7);
    copy2.deallocate(b8);
    EXPECT_TRUE(b7.empty());
    EXPECT_TRUE(b8.empty());
}

TEST_F(Free_list_allocator_test, is_movealbe)
{
    using namespace memoc;

    const Block<void>::Size_type size_in_range{ min_size_ + (max_size_ - min_size_) / 2 };

    Block<void> b1 = allocator_.allocate(size_in_range).value();
    Block<void> b1_copy = b1;

    EXPECT_FALSE(b1.empty());
    EXPECT_EQ(size_in_range, b1.size());
    EXPECT_NE(nullptr, b1.data());

    allocator_.deallocate(b1);
    Allocator moved1{ std::move(allocator_) };
    
    Block<void> b2 = allocator_.allocate(size_in_range).value();
    Block<void> b3 = moved1.allocate(size_in_range).value();

    EXPECT_FALSE(b2.empty());
    EXPECT_EQ(b1_copy.size(), b2.size());
    EXPECT_NE(b1_copy.data(), b2.data());
    EXPECT_FALSE(b3.empty());
    EXPECT_EQ(size_in_range, b3.size());
    EXPECT_NE(nullptr, b3.data());

    allocator_.deallocate(b2);
    allocator_.deallocate(b3);
    EXPECT_TRUE(b2.empty());
    EXPECT_TRUE(b3.empty());

    Allocator moved2{};

    Block<void> b4 = moved1.allocate(size_in_range).value();
    Block<void> b4_copy = b4;

    EXPECT_FALSE(b4.empty());
    EXPECT_EQ(size_in_range, b4.size());
    EXPECT_NE(nullptr, b4.data());

    allocator_.deallocate(b4);
    moved2 = std::move(moved1);

    Block<void> b5 = moved1.allocate(size_in_range).value();
    Block<void> b6 = moved2.allocate(size_in_range).value();

    EXPECT_FALSE(b5.empty());
    EXPECT_EQ(b4_copy.size(), b5.size());
    EXPECT_NE(b4_copy.data(), b5.data());
    EXPECT_FALSE(b6.empty());
    EXPECT_EQ(size_in_range, b6.size());
    EXPECT_NE(nullptr, b6.data());

    allocator_.deallocate(b5);
    allocator_.deallocate(b6);
    EXPECT_TRUE(b5.empty());
    EXPECT_TRUE(b6.empty());
}

// Stl_adapter_allocator tests

class Stl_adapter_allocator_test : public ::testing::Test {
protected:
    using Parent = memoc::Malloc_allocator;

    template <typename T>
    using Allocator = memoc::Stl_adapter_allocator<T, Parent>;
};

TEST_F(Stl_adapter_allocator_test, able_to_use_custom_allocator)
{
    std::vector<int, Allocator<int>> v1{};

    const std::int64_t number_of_allocations = 512;

    for (std::int64_t i = 0; i < number_of_allocations; ++i) {
        v1.push_back(static_cast<int>(i));
        EXPECT_EQ(static_cast<int>(i), v1[i]);
    }

    std::vector<int, Allocator<int>> v2{ v1 };

    for (std::int64_t i = 0; i < number_of_allocations; ++i) {
        v2.push_back(static_cast<int>(i));
        EXPECT_EQ(static_cast<int>(i), v2[i]);
    }

    v1.clear();
    EXPECT_TRUE(v1.empty());
    EXPECT_EQ(number_of_allocations * 2, v2.size());

    std::vector<int, Allocator<int>> v3{ std::move(v2) };

    EXPECT_TRUE(v2.empty());

    for (std::int64_t i = 0; i < number_of_allocations; ++i) {
        EXPECT_EQ(static_cast<int>(i), v3[i]);
    }

    v3.clear();
    EXPECT_TRUE(v3.empty());
}

// Stats_allocator tests

class Stats_allocator_test : public ::testing::Test {
protected:
    static constexpr std::int64_t number_of_records_ = 2;
    using Parent = memoc::Malloc_allocator;

    using Allocator = memoc::Stats_allocator<Parent, number_of_records_>;
    Allocator allocator_{};
};

TEST_F(Stats_allocator_test, stats_are_empty_when_initialized)
{
    EXPECT_EQ(nullptr, allocator_.stats_list());
    EXPECT_EQ(0, allocator_.stats_list_size());
    EXPECT_EQ(0, allocator_.total_allocated());
}

TEST_F(Stats_allocator_test, records_allocation_stats_in_cyclic_buffer)
{
    using namespace memoc;

    Block<void> b1 = allocator_.allocate(1).value();
    allocator_.deallocate(b1);

    Block<void> b2 = allocator_.allocate(2).value();
    allocator_.deallocate(b2);

    const Allocator::Record* s = allocator_.stats_list();

    EXPECT_EQ(2, allocator_.stats_list_size());

    EXPECT_NE(nullptr, s);

    EXPECT_NE(nullptr, s->record_address);
    EXPECT_NE(nullptr, s->request_address);
    EXPECT_EQ(MEMOC_SSIZEOF(Allocator::Record) + 2, s->amount);

    auto start = s->time;

    EXPECT_NE(nullptr, s->next);
    s = s->next;

    EXPECT_NE(nullptr, s->record_address);
    EXPECT_NE(nullptr, s->request_address);
    EXPECT_EQ(MEMOC_SSIZEOF(Allocator::Record) - 2, s->amount);

    auto end = s->time;

    EXPECT_EQ(nullptr, s->next);

    EXPECT_GE(end.time_since_epoch().count(), start.time_since_epoch().count());

    EXPECT_EQ(MEMOC_SSIZEOF(Allocator::Record) * 4, allocator_.total_allocated());
}

TEST_F(Stats_allocator_test, is_copyable)
{
    using namespace memoc;

    Block<void> b1 = allocator_.allocate(1).value();
    allocator_.deallocate(b1);

    Allocator copy1{ allocator_ };

    EXPECT_EQ(allocator_.stats_list_size(), copy1.stats_list_size());
    EXPECT_EQ(MEMOC_SSIZEOF(Allocator::Record) * 2, allocator_.total_allocated());
    EXPECT_EQ(MEMOC_SSIZEOF(Allocator::Record) * 2, copy1.total_allocated());

    const Allocator::Record* s1 = allocator_.stats_list();
    const Allocator::Record* s2 = copy1.stats_list();

    for (std::int64_t i = 0; i < allocator_.stats_list_size(); ++i) {
        EXPECT_EQ(s1->amount, s2->amount);
        EXPECT_NE(s1->record_address, s2->record_address);
        EXPECT_NE(s1->next, s2->next);
        EXPECT_EQ(s1->request_address, s2->request_address);
        EXPECT_EQ(s1->time, s2->time);
    }

    Allocator copy2{};
    copy2 = allocator_;

    EXPECT_EQ(allocator_.stats_list_size(), copy2.stats_list_size());
    EXPECT_EQ(MEMOC_SSIZEOF(Allocator::Record) * 2, allocator_.total_allocated());
    EXPECT_EQ(MEMOC_SSIZEOF(Allocator::Record) * 2, copy2.total_allocated());

    const Allocator::Record* s3 = allocator_.stats_list();
    const Allocator::Record* s4 = copy2.stats_list();

    for (std::int64_t i = 0; i < allocator_.stats_list_size(); ++i) {
        EXPECT_EQ(s3->amount, s4->amount);
        EXPECT_NE(s3->record_address, s4->record_address);
        EXPECT_NE(s3->next, s4->next);
        EXPECT_EQ(s3->request_address, s4->request_address);
        EXPECT_EQ(s3->time, s4->time);
    }
}

TEST_F(Stats_allocator_test, is_moveable)
{
    using namespace memoc;

    Block<void> b1 = allocator_.allocate(1).value();
    allocator_.deallocate(b1);

    Allocator moved1{ std::move(allocator_) };

    EXPECT_EQ(0, allocator_.stats_list_size());
    EXPECT_EQ(nullptr, allocator_.stats_list());
    EXPECT_EQ(0, allocator_.total_allocated());

    EXPECT_EQ(2, moved1.stats_list_size());
    EXPECT_EQ(MEMOC_SSIZEOF(Allocator::Record) * 2, moved1.total_allocated());
    EXPECT_NE(nullptr, moved1.stats_list());

    Allocator moved2{};
    moved2 = std::move(moved1);

    EXPECT_EQ(0, moved1.stats_list_size());
    EXPECT_EQ(nullptr, moved1.stats_list());
    EXPECT_EQ(0, moved1.total_allocated());

    EXPECT_EQ(2, moved2.stats_list_size());
    EXPECT_EQ(MEMOC_SSIZEOF(Allocator::Record) * 2, moved2.total_allocated());
    EXPECT_NE(nullptr, moved2.stats_list());
}

// Shared_allocator tests

class Shared_allocator_test : public ::testing::Test {
protected:
    static constexpr std::int64_t count_ = 1;
    static constexpr memoc::Block<void>::Size_type size_ = 16;
    using Parent = memoc::Stack_allocator<memoc::details::Default_global_stack_memory<count_, size_>>;

    using Allocator_default = memoc::Shared_allocator<Parent>;

    using Allocator_0 = memoc::Shared_allocator<Parent, 0>;
    using Allocator_1 = memoc::Shared_allocator<Parent, 1>;
};

TEST_F(Shared_allocator_test, saves_state_between_instances)
{
    using namespace memoc;

    const memoc::Block<void>::Size_type aligned_size = 2;

    Allocator_default a1{};
    Block<void> b1 = a1.allocate(aligned_size).value();

    Allocator_default a2{};
    Block<void> b2 = a2.allocate(aligned_size).value();
    
    EXPECT_EQ(reinterpret_cast<std::uint8_t*>(b1.data()) + aligned_size, b2.data());
}

TEST_F(Shared_allocator_test, DISABLED_not_saves_state_between_instances_when_id_is_different)
{
    using namespace memoc;

    const memoc::Block<void>::Size_type aligned_size = 2;

    Allocator_0 a1{};
    Block<void> b1 = a1.allocate(aligned_size).value();

    Allocator_1 a2{};
    Block<void> b2 = a2.allocate(aligned_size).value();

    EXPECT_NE(reinterpret_cast<std::uint8_t*>(b1.data()) + aligned_size, b2.data());
}

// Null_allocator tests

class Null_allocator_test : public ::testing::Test {
protected:
    using Allocator = memoc::Null_allocator;
    Allocator allocator_{};
};

TEST_F(Null_allocator_test, allocates_an_empty_block_have_empty_deallocation_and_not_owning_a_block)
{
    using namespace memoc;

    Block<void> b = allocator_.allocate(128).value();

    EXPECT_TRUE(b.empty());
    EXPECT_FALSE(allocator_.owns(b));

    allocator_.deallocate(b);

    EXPECT_TRUE(b.empty());
}

// Fallback_allocator tests

class Fallback_allocator_test : public ::testing::Test {
protected:
    static constexpr std::int64_t count_ = 1;
    static constexpr memoc::Block<void>::Size_type size_ = 16;
    using Primary = memoc::Stack_allocator<memoc::details::Default_global_stack_memory<count_, size_>>;
    using Fallback = memoc::Malloc_allocator;

    using Allocator = memoc::Fallback_allocator<Primary, Fallback>;
    Allocator allocator_{};
};

TEST_F(Fallback_allocator_test, DISABLED_is_copyable)
{
    using namespace memoc;

    Allocator copy1{ allocator_ };
    Block<void> b1 = copy1.allocate(size_).value();

    EXPECT_FALSE(b1.empty());
    EXPECT_EQ(size_, b1.size());
    EXPECT_NE(nullptr, b1.data());

    Allocator copy2{};
    copy2 = allocator_;
    Block<void> b2 = copy2.allocate(size_).value();

    EXPECT_FALSE(b2.empty());
    EXPECT_EQ(size_, b2.size());
    EXPECT_NE(nullptr, b2.data());
}

TEST_F(Fallback_allocator_test, DISABLED_is_moveable)
{
    using namespace memoc;

    Allocator moved1{ std::move(allocator_) };
    Block<void> b1 = moved1.allocate(size_).value();

    EXPECT_FALSE(b1.empty());
    EXPECT_EQ(size_, b1.size());
    EXPECT_NE(nullptr, b1.data());

    Allocator moved2{};
    moved2 = std::move(moved1);
    Block<void> b2 = moved2.allocate(size_).value();

    EXPECT_FALSE(b2.empty());
    EXPECT_EQ(size_, b2.size());
    EXPECT_NE(nullptr, b2.data());
}

// Allocators API tests

class Any_allocator_test : public ::testing::Test {
protected:
    using Allocator = memoc::Malloc_allocator;
    Allocator allocator_{ /*memoc::create<Allocator>()*/ };
};

TEST_F(Any_allocator_test, allocate_free_and_give_owning_indication_for_successfull_allocation)
{
    using namespace memoc;

    const Block<void>::Size_type s{ 1 };

    Block<void> b = allocator_.allocate(s).value();
    EXPECT_NE(nullptr, b.data());
    EXPECT_EQ(1, b.size());

    EXPECT_TRUE(allocator_.owns(b));

    allocator_.deallocate(b);
    EXPECT_TRUE(b.empty());
}

TEST_F(Any_allocator_test, fails_when_allocation_size_is_negative_or_when_not_in_available_memory_range)
{
    using namespace memoc;

    const Block<void>::Size_type negative_size{ -1 };
    EXPECT_EQ(Allocator_error::invalid_size, allocator_.allocate(negative_size).error());

    // Comment in order to prevent address-sanitizer error in Linux.
    //const Block<void>::Size_type not_in_range_size{ std::numeric_limits<Block<void>::Size_type>::max() };
    //EXPECT_EQ(Allocator_error::unknown, allocate(allocator_, not_in_range_size).error());
}

TEST_F(Any_allocator_test, returns_empty_non_owned_block_when_size_is_zero)
{
    using namespace memoc;

    const Block<void>::Size_type zero_size{ 0 };

    Block<void> b = allocator_.allocate(zero_size).value();
    EXPECT_TRUE(b.empty());
    EXPECT_FALSE(allocator_.owns(b));
}
