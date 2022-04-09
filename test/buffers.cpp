#include <gtest/gtest.h>

#include <utility>

#include <math/core/buffers.h>
#include <math/core/allocators.h>

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

