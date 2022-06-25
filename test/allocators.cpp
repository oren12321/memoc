#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <array>
#include <vector>
#include <chrono>
#include <utility>

#include <computoc/allocators.h>
#include <computoc/memory.h>

// Malloc_allocator tests

class Malloc_allocator_test : public ::testing::Test {
protected:
    using Allocator = math::core::allocators::Malloc_allocator;
    Allocator allocator_{};
};

TEST_F(Malloc_allocator_test, not_owns_an_empty_block)
{
    using namespace math::core::memory;

    EXPECT_FALSE(allocator_.owns(Block{}));
}

TEST_F(Malloc_allocator_test, allocates_and_deallocates_memory_successfully)
{
    using namespace math::core::memory;

    const Block::Size_type s{ 1 };

    Block b = allocator_.allocate(s);
    EXPECT_NE(nullptr, b.p);
    EXPECT_EQ(1, b.s);

    EXPECT_TRUE(allocator_.owns(b));

    allocator_.deallocate(&b);
    EXPECT_TRUE(b.empty());
}

// Stack_allocator tests

class Stack_allocator_test : public ::testing::Test {
protected:
    static constexpr std::size_t size_ = 16;

    using Allocator = math::core::allocators::Stack_allocator<size_>;
    Allocator allocator_{};
};

TEST_F(Stack_allocator_test, not_owns_an_empty_block)
{
    using namespace math::core::memory;

    EXPECT_FALSE(allocator_.owns(Block{}));
}

TEST_F(Stack_allocator_test, allocates_and_deallocates_an_arbitrary_in_range_memory_successfully)
{
    using namespace math::core::memory;

    const Block::Size_type size_in_range{ size_ / 2 };

    Block b = allocator_.allocate(size_in_range);
    EXPECT_NE(nullptr, b.p);
    EXPECT_EQ(size_in_range, b.s);

    EXPECT_TRUE(allocator_.owns(b));

    allocator_.deallocate(&b);
    EXPECT_TRUE(b.empty());
}

TEST_F(Stack_allocator_test, allocates_and_deallocates_all_available_memory_successfully)
{
    using namespace math::core::memory;

    Block b = allocator_.allocate(size_);
    EXPECT_NE(nullptr, b.p);
    EXPECT_EQ(size_, b.s);

    EXPECT_TRUE(allocator_.owns(b));

    allocator_.deallocate(&b);
    EXPECT_TRUE(b.empty());
}

TEST_F(Stack_allocator_test, reuses_the_same_memory_if_deallocating_between_two_allocations)
{
    using namespace math::core::memory;

    const Block::Size_type size_in_range{ size_ / 2 };

    Block b1 = allocator_.allocate(size_in_range);
    EXPECT_NE(nullptr, b1.p);
    EXPECT_EQ(size_in_range, b1.s);
    EXPECT_TRUE(allocator_.owns(b1));

    Block b1_copy{ b1 };

    allocator_.deallocate(&b1);
    EXPECT_TRUE(b1.empty());

    Block b2 = allocator_.allocate(size_in_range);
    EXPECT_NE(nullptr, b2.p);
    EXPECT_EQ(size_in_range, b2.s);
    EXPECT_TRUE(allocator_.owns(b2));
    EXPECT_EQ(b1_copy.p, b2.p);
    EXPECT_EQ(b1_copy.s, b2.s);

    allocator_.deallocate(&b2);
    EXPECT_TRUE(b2.empty());
}

TEST_F(Stack_allocator_test, fails_to_allocate_memory_bigger_than_memory_size)
{
    using namespace math::core::memory;

    Block b = allocator_.allocate(size_ + 1);
    EXPECT_TRUE(b.empty());
}

TEST_F(Stack_allocator_test, is_copyable)
{
    using namespace math::core::memory;

    Allocator copy1{ allocator_ };

    Block b1 = allocator_.allocate(size_);
    Block b2 = copy1.allocate(size_);

    EXPECT_FALSE(b1.empty());
    EXPECT_FALSE(b2.empty());
    EXPECT_EQ(size_, b1.s);
    EXPECT_EQ(size_, b2.s);
    EXPECT_NE(b1.p, b2.p);

    Allocator copy2{};
    copy2 = allocator_;

    Block b3 = copy2.allocate(size_);

    EXPECT_FALSE(b1.empty());
    EXPECT_FALSE(b3.empty());
    EXPECT_EQ(size_, b1.s);
    EXPECT_EQ(size_, b3.s);
    EXPECT_NE(b1.p, b3.p);
}

TEST_F(Stack_allocator_test, is_movealbe)
{
    using namespace math::core::memory;

    Allocator moved1{ std::move(allocator_) };

    Block b1 = allocator_.allocate(size_);
    Block b2 = moved1.allocate(size_);

    EXPECT_TRUE(b1.empty());
    EXPECT_FALSE(b2.empty());
    EXPECT_EQ(size_, b2.s);
    EXPECT_NE(nullptr, b2.p);

    Allocator moved2{};
    moved2 = std::move(moved1);

    Block b3 = moved1.allocate(size_);
    Block b4 = moved2.allocate(size_);

    EXPECT_TRUE(b3.empty());
    EXPECT_FALSE(b4.empty());
    EXPECT_EQ(size_, b4.s);
    EXPECT_NE(nullptr, b4.p);
}

// Free_list_allocator tests

class Free_list_allocator_test : public ::testing::Test {
protected:
    static constexpr std::size_t min_size_ = 16;
    static constexpr std::size_t max_size_ = 32;
    static constexpr std::size_t max_list_size_ = 2;
    using Parent = math::core::allocators::Malloc_allocator;

    using Allocator = math::core::allocators::Free_list_allocator<Parent, min_size_, max_size_, max_list_size_>;
    Allocator allocator_{};
};

TEST_F(Free_list_allocator_test, not_owns_an_empty_block)
{
    using namespace math::core::memory;

    EXPECT_FALSE(allocator_.owns(Block{}));
}

TEST_F(Free_list_allocator_test, allocates_and_deallocates_an_arbitrary_not_in_range_memory_successfully_using_parent_allocator)
{
    using namespace math::core::memory;

    const Block::Size_type size_out_of_range{ max_size_ + 1 };

    Block b = allocator_.allocate(size_out_of_range);
    EXPECT_NE(nullptr, b.p);
    EXPECT_EQ(size_out_of_range, b.s);

    EXPECT_TRUE(allocator_.owns(b));

    allocator_.deallocate(&b);
    EXPECT_TRUE(b.empty());
}

TEST_F(Free_list_allocator_test, reuses_the_same_memory_if_deallocating_in_memory_range)
{
    using namespace math::core::memory;

    const Block::Size_type size_in_range{ min_size_ + (max_size_ - min_size_) / 2 };

    std::array<Block, max_list_size_> saved_blocks{};

    for (std::size_t i = 0; i < max_list_size_; ++i)
    {
        Block b = allocator_.allocate(size_in_range);
        EXPECT_NE(nullptr, b.p);
        EXPECT_EQ(size_in_range, b.s);
        EXPECT_TRUE(allocator_.owns(b));

        saved_blocks[i] = b;
    }

    for (std::array<Block, max_list_size_>::const_reverse_iterator it = saved_blocks.rbegin(); it != saved_blocks.rend(); ++it)
    {
        Block b_copy{ *it };
        allocator_.deallocate(&b_copy);
        EXPECT_TRUE(b_copy.empty());
    }

    for (const auto& saved_block : saved_blocks)
    {
        Block b = allocator_.allocate(size_in_range);
        EXPECT_NE(nullptr, b.p);
        EXPECT_EQ(size_in_range, b.s);
        EXPECT_TRUE(allocator_.owns(b));
        EXPECT_EQ(saved_block.p, b.p);
        EXPECT_EQ(saved_block.s, b.s);
    }

    // Test cleanup since free list does not actually release memory
    Parent p{};
    for (auto& saved_block : saved_blocks)
    {
        p.deallocate(&saved_block);
        EXPECT_TRUE(saved_block.empty());
    }
}

TEST_F(Free_list_allocator_test, is_copyable)
{
    using namespace math::core::memory;

    const Block::Size_type size_in_range{ min_size_ + (max_size_ - min_size_) / 2 };

    Allocator copy1{ allocator_ };

    Block b1 = allocator_.allocate(size_in_range);
    Block b2 = copy1.allocate(size_in_range);

    EXPECT_FALSE(b1.empty());
    EXPECT_FALSE(b2.empty());
    EXPECT_EQ(size_in_range, b1.s);
    EXPECT_EQ(size_in_range, b2.s);
    EXPECT_NE(b1.p, b2.p);

    Block b1_copy = b1;
    allocator_.deallocate(&b1);
    Block b3 = allocator_.allocate(size_in_range);
    Block b2_copy = b2;
    copy1.deallocate(&b2);
    Block b4 = copy1.allocate(size_in_range);

    EXPECT_EQ(b1_copy.s, b3.s);
    EXPECT_EQ(b1_copy.p, b3.p);
    EXPECT_EQ(b2_copy.s, b4.s);
    EXPECT_EQ(b2_copy.p, b4.p);

    allocator_.deallocate(&b3);
    copy1.deallocate(&b4);
    EXPECT_TRUE(b3.empty());
    EXPECT_TRUE(b4.empty());

    Allocator copy2{};
    copy2 = allocator_;

    Block b5 = allocator_.allocate(size_in_range);
    Block b6 = copy2.allocate(size_in_range);

    EXPECT_FALSE(b5.empty());
    EXPECT_FALSE(b6.empty());
    EXPECT_EQ(size_in_range, b5.s);
    EXPECT_EQ(size_in_range, b6.s);
    EXPECT_NE(b5.p, b6.p);

    Block b5_copy = b5;
    allocator_.deallocate(&b5);
    Block b7 = allocator_.allocate(size_in_range);
    Block b6_copy = b6;
    copy2.deallocate(&b6);
    Block b8 = copy2.allocate(size_in_range);

    EXPECT_EQ(b5_copy.s, b7.s);
    EXPECT_EQ(b5_copy.p, b7.p);
    EXPECT_EQ(b6_copy.s, b8.s);
    EXPECT_EQ(b6_copy.p, b8.p);

    allocator_.deallocate(&b7);
    copy2.deallocate(&b8);
    EXPECT_TRUE(b7.empty());
    EXPECT_TRUE(b8.empty());
}

TEST_F(Free_list_allocator_test, is_movealbe)
{
    using namespace math::core::memory;

    const Block::Size_type size_in_range{ min_size_ + (max_size_ - min_size_) / 2 };

    Block b1 = allocator_.allocate(size_in_range);
    Block b1_copy = b1;

    EXPECT_FALSE(b1.empty());
    EXPECT_EQ(size_in_range, b1.s);
    EXPECT_NE(nullptr, b1.p);

    allocator_.deallocate(&b1);
    Allocator moved1{ std::move(allocator_) };
    
    Block b2 = allocator_.allocate(size_in_range);
    Block b3 = moved1.allocate(size_in_range);

    EXPECT_FALSE(b2.empty());
    EXPECT_EQ(b1_copy.s, b2.s);
    EXPECT_NE(b1_copy.p, b2.p);
    EXPECT_FALSE(b3.empty());
    EXPECT_EQ(size_in_range, b3.s);
    EXPECT_NE(nullptr, b3.p);

    allocator_.deallocate(&b2);
    allocator_.deallocate(&b3);
    EXPECT_TRUE(b2.empty());
    EXPECT_TRUE(b3.empty());

    Allocator moved2{};

    Block b4 = moved1.allocate(size_in_range);
    Block b4_copy = b4;

    EXPECT_FALSE(b4.empty());
    EXPECT_EQ(size_in_range, b4.s);
    EXPECT_NE(nullptr, b4.p);

    allocator_.deallocate(&b4);
    moved2 = std::move(moved1);

    Block b5 = moved1.allocate(size_in_range);
    Block b6 = moved2.allocate(size_in_range);

    EXPECT_FALSE(b5.empty());
    EXPECT_EQ(b4_copy.s, b5.s);
    EXPECT_NE(b4_copy.p, b5.p);
    EXPECT_FALSE(b6.empty());
    EXPECT_EQ(size_in_range, b6.s);
    EXPECT_NE(nullptr, b6.p);

    allocator_.deallocate(&b5);
    allocator_.deallocate(&b6);
    EXPECT_TRUE(b5.empty());
    EXPECT_TRUE(b6.empty());
}

// Stl_adapter_allocator tests

class Stl_adapter_allocator_test : public ::testing::Test {
protected:
    using Parent = math::core::allocators::Malloc_allocator;

    template <typename T>
    using Allocator = math::core::allocators::Stl_adapter_allocator<T, Parent>;
};

TEST_F(Stl_adapter_allocator_test, able_to_use_custom_allocator)
{
    std::vector<int, Allocator<int>> v1{};

    const std::size_t number_of_allocations = 512;

    for (std::size_t i = 0; i < number_of_allocations; ++i) {
        v1.push_back(static_cast<int>(i));
        EXPECT_EQ(static_cast<int>(i), v1[i]);
    }

    std::vector<int, Allocator<int>> v2{ v1 };

    for (std::size_t i = 0; i < number_of_allocations; ++i) {
        v2.push_back(static_cast<int>(i));
        EXPECT_EQ(static_cast<int>(i), v2[i]);
    }

    v1.clear();
    EXPECT_TRUE(v1.empty());
    EXPECT_EQ(number_of_allocations * 2, v2.size());

    std::vector<int, Allocator<int>> v3{ std::move(v2) };

    EXPECT_TRUE(v2.empty());

    for (std::size_t i = 0; i < number_of_allocations; ++i) {
        EXPECT_EQ(static_cast<int>(i), v3[i]);
    }

    v3.clear();
    EXPECT_TRUE(v3.empty());
}

// Stats_allocator tests

class Stats_allocator_test : public ::testing::Test {
protected:
    static constexpr std::size_t number_of_records_ = 2;
    using Parent = math::core::allocators::Malloc_allocator;

    using Allocator = math::core::allocators::Stats_allocator<Parent, number_of_records_>;
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
    using namespace math::core::memory;

    Block b1 = allocator_.allocate(1);
    allocator_.deallocate(&b1);

    Block b2 = allocator_.allocate(2);
    allocator_.deallocate(&b2);

    const Allocator::Record* s = allocator_.stats_list();

    EXPECT_EQ(2, allocator_.stats_list_size());

    EXPECT_NE(nullptr, s);

    EXPECT_NE(nullptr, s->record_address);
    EXPECT_NE(nullptr, s->request_address);
    EXPECT_EQ(sizeof(Allocator::Record) + 2, s->amount);

    auto start = s->time;

    EXPECT_NE(nullptr, s->next);
    s = s->next;

    EXPECT_NE(nullptr, s->record_address);
    EXPECT_NE(nullptr, s->request_address);
    EXPECT_EQ(sizeof(Allocator::Record) - 2, s->amount);

    auto end = s->time;

    EXPECT_EQ(nullptr, s->next);

    EXPECT_GE(end.time_since_epoch().count(), start.time_since_epoch().count());

    EXPECT_EQ(sizeof(Allocator::Record) * 4, allocator_.total_allocated());
}

TEST_F(Stats_allocator_test, is_copyable)
{
    using namespace math::core::memory;

    Block b1 = allocator_.allocate(1);
    allocator_.deallocate(&b1);

    Allocator copy1{ allocator_ };

    EXPECT_EQ(allocator_.stats_list_size(), copy1.stats_list_size());
    EXPECT_EQ(sizeof(Allocator::Record) * 2, allocator_.total_allocated());
    EXPECT_EQ(sizeof(Allocator::Record) * 2, copy1.total_allocated());

    const Allocator::Record* s1 = allocator_.stats_list();
    const Allocator::Record* s2 = copy1.stats_list();

    for (std::size_t i = 0; i < allocator_.stats_list_size(); ++i) {
        EXPECT_EQ(s1->amount, s2->amount);
        EXPECT_NE(s1->record_address, s2->record_address);
        EXPECT_NE(s1->next, s2->next);
        EXPECT_EQ(s1->request_address, s2->request_address);
        EXPECT_EQ(s1->time, s2->time);
    }

    Allocator copy2{};
    copy2 = allocator_;

    EXPECT_EQ(allocator_.stats_list_size(), copy2.stats_list_size());
    EXPECT_EQ(sizeof(Allocator::Record) * 2, allocator_.total_allocated());
    EXPECT_EQ(sizeof(Allocator::Record) * 2, copy2.total_allocated());

    const Allocator::Record* s3 = allocator_.stats_list();
    const Allocator::Record* s4 = copy2.stats_list();

    for (std::size_t i = 0; i < allocator_.stats_list_size(); ++i) {
        EXPECT_EQ(s3->amount, s4->amount);
        EXPECT_NE(s3->record_address, s4->record_address);
        EXPECT_NE(s3->next, s4->next);
        EXPECT_EQ(s3->request_address, s4->request_address);
        EXPECT_EQ(s3->time, s4->time);
    }
}

TEST_F(Stats_allocator_test, is_moveable)
{
    using namespace math::core::memory;

    Block b1 = allocator_.allocate(1);
    allocator_.deallocate(&b1);

    Allocator moved1{ std::move(allocator_) };

    EXPECT_EQ(0, allocator_.stats_list_size());
    EXPECT_EQ(nullptr, allocator_.stats_list());
    EXPECT_EQ(0, allocator_.total_allocated());

    EXPECT_EQ(2, moved1.stats_list_size());
    EXPECT_EQ(sizeof(Allocator::Record) * 2, moved1.total_allocated());
    EXPECT_NE(nullptr, moved1.stats_list());

    Allocator moved2{};
    moved2 = std::move(moved1);

    EXPECT_EQ(0, moved1.stats_list_size());
    EXPECT_EQ(nullptr, moved1.stats_list());
    EXPECT_EQ(0, moved1.total_allocated());

    EXPECT_EQ(2, moved2.stats_list_size());
    EXPECT_EQ(sizeof(Allocator::Record) * 2, moved2.total_allocated());
    EXPECT_NE(nullptr, moved2.stats_list());
}

// Shared_allocator tests

class Shared_allocator_test : public ::testing::Test {
protected:
    static constexpr std::size_t size_ = 16;
    using Parent = math::core::allocators::Stack_allocator<size_>;

    using Allocator_default = math::core::allocators::Shared_allocator<Parent>;

    using Allocator_0 = math::core::allocators::Shared_allocator<Parent, 0>;
    using Allocator_1 = math::core::allocators::Shared_allocator<Parent, 1>;
};

TEST_F(Shared_allocator_test, saves_state_between_instances)
{
    using namespace math::core::memory;

    const std::size_t aligned_size = 2;

    Allocator_default a1{};
    Block b1 = a1.allocate(aligned_size);

    Allocator_default a2{};
    Block b2 = a2.allocate(aligned_size);
    
    EXPECT_EQ(reinterpret_cast<std::uint8_t*>(b1.p) + aligned_size, b2.p);
}

TEST_F(Shared_allocator_test, not_saves_state_between_instances_when_id_is_different)
{
    using namespace math::core::memory;

    const std::size_t aligned_size = 2;

    Allocator_0 a1{};
    Block b1 = a1.allocate(aligned_size);

    Allocator_1 a2{};
    Block b2 = a2.allocate(aligned_size);

    EXPECT_NE(reinterpret_cast<std::uint8_t*>(b1.p) + aligned_size, b2.p);
}

// Fallback_allocator tests

class Fallback_allocator_test : public ::testing::Test {
protected:
    static constexpr std::size_t size_ = 16;
    using Primary = math::core::allocators::Stack_allocator<size_>;
    using Fallback = math::core::allocators::Malloc_allocator;

    using Allocator = math::core::allocators::Fallback_allocator<Primary, Fallback>;
    Allocator allocator_{};
};

TEST_F(Fallback_allocator_test, is_copyable)
{
    using namespace math::core::memory;

    Allocator copy1{ allocator_ };
    Block b1 = copy1.allocate(size_);

    EXPECT_FALSE(b1.empty());
    EXPECT_EQ(size_, b1.s);
    EXPECT_NE(nullptr, b1.p);

    Allocator copy2{};
    copy2 = allocator_;
    Block b2 = copy2.allocate(size_);

    EXPECT_FALSE(b2.empty());
    EXPECT_EQ(size_, b2.s);
    EXPECT_NE(nullptr, b2.p);
}

TEST_F(Fallback_allocator_test, is_moveable)
{
    using namespace math::core::memory;

    Allocator moved1{ std::move(allocator_) };
    Block b1 = moved1.allocate(size_);

    EXPECT_FALSE(b1.empty());
    EXPECT_EQ(size_, b1.s);
    EXPECT_NE(nullptr, b1.p);

    Allocator moved2{};
    moved2 = std::move(moved1);
    Block b2 = moved2.allocate(size_);

    EXPECT_FALSE(b2.empty());
    EXPECT_EQ(size_, b2.s);
    EXPECT_NE(nullptr, b2.p);
}

TEST(Unique_ptr_test, can_be_created_with_custom_allocator)
{
    using namespace math::core::allocators;

    {
        std::unique_ptr<int, aux::Custom_deleter<Malloc_allocator, int>> p =
            aux::make_unique<Malloc_allocator, int>(1);
        EXPECT_EQ(1, *p);
    }

    Malloc_allocator allocator{};
    math::core::memory::Block b = allocator.allocate(sizeof(int));
    int* r = math::core::memory::aux::construct_at<int>(reinterpret_cast<int*>(b.p), 1);
    {
        std::unique_ptr<int, aux::Custom_deleter<Malloc_allocator, int>> p = aux::make_unique<Malloc_allocator, int>(r);
        EXPECT_EQ(1, *p);
    }
}

TEST(Shared_ptr_test, can_be_created_with_custom_allocator)
{
    using namespace math::core::allocators;

    std::shared_ptr<int> p1 = nullptr;
    {
        std::shared_ptr<int> p2 =
            aux::make_shared<Malloc_allocator, int>(1);
        EXPECT_EQ(1, *p2);
        p1 = p2;
        EXPECT_EQ(1, *p1);
        EXPECT_EQ(2, p1.use_count());
        EXPECT_EQ(2, p2.use_count());
    }
    EXPECT_EQ(1, *p1);
    EXPECT_EQ(1, p1.use_count());

    Malloc_allocator allocator{};
    math::core::memory::Block b = allocator.allocate(sizeof(int));
    int* r = math::core::memory::aux::construct_at<int>(reinterpret_cast<int*>(b.p), 1);
    std::shared_ptr<int> p3 = nullptr;
    {
        std::shared_ptr<int> p4 =
            aux::make_shared<Malloc_allocator, int>(r);
        EXPECT_EQ(1, *p4);
        p3 = p4;
        EXPECT_EQ(1, *p4);
        EXPECT_EQ(2, p3.use_count());
        EXPECT_EQ(2, p4.use_count());
    }
    EXPECT_EQ(1, *p3);
    EXPECT_EQ(1, p3.use_count());
}
