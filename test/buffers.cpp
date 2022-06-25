#include <gtest/gtest.h>

#include <utility>

#include <computoc/buffers.h>
#include <computoc/allocators.h>
#include <computoc/memory.h>

// Stack_buffer tests

TEST(Stack_buffer_test, usable_when_initialized_with_valid_size)
{
    using namespace math::core::buffers;

    Stack_buffer buff{ 2 };

    EXPECT_TRUE(buff.usable());
    EXPECT_FALSE(buff.data().empty());
    EXPECT_NE(nullptr, buff.data().p);
    EXPECT_EQ(2, buff.data().s);
}

TEST(Stack_buffer_test, not_usable_when_initialized_with_invalid_size)
{
    using namespace math::core::buffers;

    Stack_buffer buff{ 4 };

    EXPECT_FALSE(buff.usable());
    EXPECT_TRUE(buff.data().empty());
}

TEST(Stack_buffer_test, is_copyable)
{
    using namespace math::core::buffers;

    Stack_buffer buff1{ 2 };
    Stack_buffer buff2{ buff1 };

    EXPECT_TRUE(buff1.usable());
    EXPECT_FALSE(buff1.data().empty());
    EXPECT_NE(nullptr, buff1.data().p);
    EXPECT_EQ(2, buff1.data().s);

    EXPECT_TRUE(buff2.usable());
    EXPECT_FALSE(buff2.data().empty());
    EXPECT_NE(nullptr, buff2.data().p);
    EXPECT_EQ(2, buff2.data().s);

    EXPECT_NE(buff1.data().p, buff2.data().p);
    EXPECT_EQ(buff1.data().s, buff2.data().s);

    Stack_buffer buff3{ 2 };
    buff3 = buff2;

    EXPECT_TRUE(buff3.usable());
    EXPECT_FALSE(buff3.data().empty());
    EXPECT_NE(nullptr, buff3.data().p);
    EXPECT_EQ(2, buff3.data().s);

    EXPECT_NE(buff2.data().p, buff3.data().p);
    EXPECT_EQ(buff2.data().s, buff3.data().s);
}

TEST(Stack_buffer_test, is_moveable)
{
    using namespace math::core::buffers;

    Stack_buffer buff1{ 2 };
    Stack_buffer buff2{ std::move(buff1) };

    EXPECT_FALSE(buff1.usable());
    EXPECT_TRUE(buff1.data().empty());

    EXPECT_TRUE(buff2.usable());
    EXPECT_FALSE(buff2.data().empty());
    EXPECT_NE(nullptr, buff2.data().p);
    EXPECT_EQ(2, buff2.data().s);

    EXPECT_NE(buff1.data().p, buff2.data().p);
    EXPECT_NE(buff1.data().s, buff2.data().s);

    Stack_buffer buff3{ 2 };
    buff3 = std::move(buff2);

    EXPECT_FALSE(buff2.usable());
    EXPECT_TRUE(buff2.data().empty());

    EXPECT_NE(buff2.data().p, buff3.data().p);
    EXPECT_NE(buff2.data().s, buff3.data().s);
}

TEST(Stack_buffer_test, can_be_initalized_with_data)
{
    using namespace math::core::buffers;

    const int data[2] = {1, 2};

    Stack_buffer<2 * sizeof(int)> buff1{ 2 * sizeof(int), data };

    EXPECT_TRUE(buff1.usable());
    EXPECT_FALSE(buff1.data().empty());
    EXPECT_NE(nullptr, buff1.data().p);
    EXPECT_EQ(2 * sizeof(int), buff1.data().s);

    int* data1 = reinterpret_cast<int*>(buff1.data().p);
    EXPECT_EQ(data[0], data1[0]);
    EXPECT_EQ(data[1], data1[1]);

    Stack_buffer<2 * sizeof(int)> copy1{buff1};

    EXPECT_TRUE(copy1.usable());
    EXPECT_FALSE(copy1.data().empty());
    EXPECT_NE(nullptr, copy1.data().p);
    EXPECT_EQ(2 * sizeof(int), copy1.data().s);

    EXPECT_NE(buff1.data().p, copy1.data().p);
    EXPECT_EQ(buff1.data().s, copy1.data().s);

    int* copy_data1 = reinterpret_cast<int*>(copy1.data().p);
    EXPECT_EQ(data[0], copy_data1[0]);
    EXPECT_EQ(data[1], copy_data1[1]);

    Stack_buffer<2 * sizeof(int)> moved1{std::move(buff1)};

    EXPECT_FALSE(buff1.usable());
    EXPECT_TRUE(buff1.data().empty());

    EXPECT_TRUE(moved1.usable());
    EXPECT_FALSE(moved1.data().empty());
    EXPECT_NE(nullptr, moved1.data().p);
    EXPECT_EQ(2 * sizeof(int), moved1.data().s);

    int* moved_data1 = reinterpret_cast<int*>(moved1.data().p);
    EXPECT_EQ(data[0], moved_data1[0]);
    EXPECT_EQ(data[1], moved_data1[1]);
}

// Allocated_buffer tests

TEST(Allocated_buffer_test, usable_when_initialized_with_valid_size)
{
    using namespace math::core::buffers;
    using namespace math::core::allocators;

    Allocated_buffer<Malloc_allocator> buff{ 2 };

    EXPECT_TRUE(buff.usable());
    EXPECT_FALSE(buff.data().empty());
    EXPECT_NE(nullptr, buff.data().p);
    EXPECT_EQ(2, buff.data().s);
}

TEST(Allocated_buffer_test, not_usable_when_initialized_with_invalid_size)
{
    using namespace math::core::buffers;
    using namespace math::core::allocators;

    Allocated_buffer<Stack_allocator<2>> buff{ 4 };

    EXPECT_FALSE(buff.usable());
    EXPECT_TRUE(buff.data().empty());
}

TEST(Allocated_buffer_test, is_copyable)
{
    using namespace math::core::buffers;
    using namespace math::core::allocators;

    Allocated_buffer<Malloc_allocator> buff1{ 2 };
    Allocated_buffer<Malloc_allocator> buff2{ buff1 };

    EXPECT_TRUE(buff1.usable());
    EXPECT_FALSE(buff1.data().empty());
    EXPECT_NE(nullptr, buff1.data().p);
    EXPECT_EQ(2, buff1.data().s);

    EXPECT_TRUE(buff2.usable());
    EXPECT_FALSE(buff2.data().empty());
    EXPECT_NE(nullptr, buff2.data().p);
    EXPECT_EQ(2, buff2.data().s);

    EXPECT_NE(buff1.data().p, buff2.data().p);
    EXPECT_EQ(buff1.data().s, buff2.data().s);

    Allocated_buffer<Malloc_allocator> buff3{ 2 };
    buff3 = buff2;

    EXPECT_TRUE(buff3.usable());
    EXPECT_FALSE(buff3.data().empty());
    EXPECT_NE(nullptr, buff3.data().p);
    EXPECT_EQ(2, buff3.data().s);

    EXPECT_NE(buff2.data().p, buff3.data().p);
    EXPECT_EQ(buff2.data().s, buff3.data().s);
}

TEST(Allocated_buffer_test, is_moveable)
{
    using namespace math::core::buffers;
    using namespace math::core::allocators;

    Allocated_buffer<Malloc_allocator> buff1{ 2 };
    Allocated_buffer<Malloc_allocator> buff2{ std::move(buff1) };

    EXPECT_FALSE(buff1.usable());
    EXPECT_TRUE(buff1.data().empty());

    EXPECT_TRUE(buff2.usable());
    EXPECT_FALSE(buff2.data().empty());
    EXPECT_NE(nullptr, buff2.data().p);
    EXPECT_EQ(2, buff2.data().s);

    EXPECT_NE(buff1.data().p, buff2.data().p);
    EXPECT_NE(buff1.data().s, buff2.data().s);

    Allocated_buffer<Malloc_allocator> buff3{ 2 };
    buff3 = std::move(buff2);

    EXPECT_FALSE(buff2.usable());
    EXPECT_TRUE(buff2.data().empty());

    EXPECT_NE(buff2.data().p, buff3.data().p);
    EXPECT_NE(buff2.data().s, buff3.data().s);
}

TEST(Allocated_buffer_test, can_be_initalized_with_data)
{
    using namespace math::core::buffers;
    using namespace math::core::allocators;

    const int data[2] = {1, 2};

    Allocated_buffer<Malloc_allocator> buff1{ 2 * sizeof(int), data };

    EXPECT_TRUE(buff1.usable());
    EXPECT_FALSE(buff1.data().empty());
    EXPECT_NE(nullptr, buff1.data().p);
    EXPECT_EQ(2 * sizeof(int), buff1.data().s);

    int* data1 = reinterpret_cast<int*>(buff1.data().p);
    EXPECT_EQ(data[0], data1[0]);
    EXPECT_EQ(data[1], data1[1]);

    Allocated_buffer<Malloc_allocator> copy1{buff1};

    EXPECT_TRUE(copy1.usable());
    EXPECT_FALSE(copy1.data().empty());
    EXPECT_NE(nullptr, copy1.data().p);
    EXPECT_EQ(2 * sizeof(int), copy1.data().s);

    EXPECT_NE(buff1.data().p, copy1.data().p);
    EXPECT_EQ(buff1.data().s, copy1.data().s);

    int* copy_data1 = reinterpret_cast<int*>(copy1.data().p);
    EXPECT_EQ(data[0], copy_data1[0]);
    EXPECT_EQ(data[1], copy_data1[1]);

    Allocated_buffer<Malloc_allocator> moved1{std::move(buff1)};

    EXPECT_FALSE(buff1.usable());
    EXPECT_TRUE(buff1.data().empty());

    EXPECT_TRUE(moved1.usable());
    EXPECT_FALSE(moved1.data().empty());
    EXPECT_NE(nullptr, moved1.data().p);
    EXPECT_EQ(2 * sizeof(int), moved1.data().s);

    int* moved_data1 = reinterpret_cast<int*>(moved1.data().p);
    EXPECT_EQ(data[0], moved_data1[0]);
    EXPECT_EQ(data[1], moved_data1[1]);
}

// Fallback_buffer tests

TEST(Fallback_buffer_test, uses_the_first_buffer_when_usable)
{
    using namespace math::core::buffers;
    using namespace math::core::allocators;

    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Malloc_allocator, true>> buff{ 2 };

    EXPECT_TRUE(buff.usable());
    EXPECT_FALSE(buff.data().empty());
    EXPECT_NE(nullptr, buff.data().p);
    EXPECT_EQ(2, buff.data().s);
}

TEST(Fallback_buffer_test, uses_the_second_buffer_when_usable)
{
    using namespace math::core::buffers;
    using namespace math::core::allocators;

    // Required size invalid for first allocator
    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Malloc_allocator, true>> buff{ 4 };

    EXPECT_TRUE(buff.usable());
    EXPECT_FALSE(buff.data().empty());
    EXPECT_NE(nullptr, buff.data().p);
    EXPECT_EQ(4, buff.data().s);
}

TEST(Fallback_buffer_test, not_usable_when_initialized_with_invalid_size_for_both_buffers)
{
    using namespace math::core::buffers;
    using namespace math::core::allocators;

    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Stack_allocator<2>, true>> buff{ 4 };

    EXPECT_FALSE(buff.usable());
    EXPECT_TRUE(buff.data().empty());
}

// Just for compilation check
TEST(Fallback_buffer_test, is_copyable)
{
    using namespace math::core::buffers;
    using namespace math::core::allocators;

    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Stack_allocator<2>, true>> buff1{ 4 };
    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Stack_allocator<2>, true>> buff2{ buff1 };
    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Stack_allocator<2>, true>> buff3{ 4 };
    buff3 = buff2;
}

// Just for compilation check
TEST(Fallback_buffer_test, is_moveable)
{
    using namespace math::core::buffers;
    using namespace math::core::allocators;

    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Stack_allocator<2>, true>> buff1{ 4 };
    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Stack_allocator<2>, true>> buff2{ std::move(buff1) };
    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Stack_allocator<2>, true>> buff3{ 4 };
    buff3 = std::move(buff2);
}

TEST(Fallback_buffer_test, can_be_initalized_with_data)
{
    using namespace math::core::buffers;
    using namespace math::core::allocators;

    const int data[2] = { 1, 2 };

    Fallback_buffer<Stack_buffer<2 * sizeof(int)>, Allocated_buffer<Malloc_allocator, true>> buff1{ 2 * sizeof(int), data };

    EXPECT_TRUE(buff1.usable());
    EXPECT_FALSE(buff1.data().empty());
    EXPECT_NE(nullptr, buff1.data().p);
    EXPECT_EQ(2 * sizeof(int), buff1.data().s);

    int* data1 = reinterpret_cast<int*>(buff1.data().p);
    EXPECT_EQ(data[0], data1[0]);
    EXPECT_EQ(data[1], data1[1]);

    Fallback_buffer<Stack_buffer<2 * sizeof(int)>, Allocated_buffer<Malloc_allocator, true>> copy1{ buff1 };

    EXPECT_TRUE(copy1.usable());
    EXPECT_FALSE(copy1.data().empty());
    EXPECT_NE(nullptr, copy1.data().p);
    EXPECT_EQ(2 * sizeof(int), copy1.data().s);

    int* copy_data1 = reinterpret_cast<int*>(copy1.data().p);
    EXPECT_EQ(data[0], copy_data1[0]);
    EXPECT_EQ(data[1], copy_data1[1]);

    Fallback_buffer<Stack_buffer<2 * sizeof(int)>, Allocated_buffer<Malloc_allocator, true>> moved1{ std::move(buff1) };

    EXPECT_FALSE(buff1.usable());
    EXPECT_TRUE(buff1.data().empty());

    EXPECT_TRUE(moved1.usable());
    EXPECT_FALSE(moved1.data().empty());
    EXPECT_NE(nullptr, moved1.data().p);
    EXPECT_EQ(2 * sizeof(int), moved1.data().s);

    int* moved_data1 = reinterpret_cast<int*>(moved1.data().p);
    EXPECT_EQ(data[0], moved_data1[0]);
    EXPECT_EQ(data[1], moved_data1[1]);

    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Malloc_allocator, true>> buff2{ 2 * sizeof(int), data };

    EXPECT_TRUE(buff2.usable());
    EXPECT_FALSE(buff2.data().empty());
    EXPECT_NE(nullptr, buff2.data().p);
    EXPECT_EQ(2 * sizeof(int), buff2.data().s);

    int* data2 = reinterpret_cast<int*>(buff2.data().p);
    EXPECT_EQ(data[0], data2[0]);
    EXPECT_EQ(data[1], data2[1]);

    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Malloc_allocator, true>> copy2{ buff2 };

    EXPECT_TRUE(copy2.usable());
    EXPECT_FALSE(copy2.data().empty());
    EXPECT_NE(nullptr, copy2.data().p);
    EXPECT_EQ(2 * sizeof(int), copy2.data().s);

    int* copy_data2 = reinterpret_cast<int*>(copy2.data().p);
    EXPECT_EQ(data[0], copy_data2[0]);
    EXPECT_EQ(data[1], copy_data2[1]);

    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Malloc_allocator, true>> moved2{ std::move(buff2) };

    EXPECT_FALSE(buff2.usable());
    EXPECT_TRUE(buff2.data().empty());

    EXPECT_TRUE(moved2.usable());
    EXPECT_FALSE(moved2.data().empty());
    EXPECT_NE(nullptr, moved2.data().p);
    EXPECT_EQ(2 * sizeof(int), moved2.data().s);

    int* moved_data2 = reinterpret_cast<int*>(moved2.data().p);
    EXPECT_EQ(data[0], moved_data2[0]);
    EXPECT_EQ(data[1], moved_data2[1]);
}

// Typed_buffer tests

TEST(TYped_buffer_test, can_be_initialized_with_specific_data_type)
{
    using namespace math::core::buffers;
    using namespace math::core::memory;
    using namespace math::core::allocators;

    const int data[2] = {1, 2};

    Typed_buffer<int, Allocated_buffer<Malloc_allocator>> buff{2, data};

    EXPECT_TRUE(buff.usable());

    Typed_block b = buff.data();

    EXPECT_FALSE(b.empty());
    EXPECT_NE(nullptr, b.p);
    EXPECT_EQ(2, b.s);

    EXPECT_EQ(data[0], b.p[0]);
    EXPECT_EQ(data[1], b.p[1]);
}

