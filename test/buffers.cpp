#include <gtest/gtest.h>

#include <math/core/buffers.h>
#include <math/core/allocators.h>

// Stack_buffer tests

TEST(Stack_buffer_test, usable_when_initialized_with_valid_size)
{
    using namespace math::core::buffers;

    Stack_buffer buff{2};

    EXPECT_TRUE(buff.usable());
    EXPECT_FALSE(buff.data().empty());
    EXPECT_NE(nullptr, buff.data().p);
    EXPECT_EQ(2, buff.data().s);
}

TEST(Stack_buffer_test, not_usable_when_initialized_with_invalid_size)
{
    using namespace math::core::buffers;

    Stack_buffer buff{4};

    EXPECT_FALSE(buff.usable());
    EXPECT_TRUE(buff.data().empty());
}

// Allocated_buffer tests

TEST(Allocated_buffer_test, usable_when_initialized_with_valid_size)
{
    using namespace math::core::buffers;
    using namespace math::core::allocators;

    Allocated_buffer<Malloc_allocator> buff{2};

    EXPECT_TRUE(buff.usable());
    EXPECT_FALSE(buff.data().empty());
    EXPECT_NE(nullptr, buff.data().p);
    EXPECT_EQ(2, buff.data().s);
}

TEST(Allocated_buffer_test, not_usable_when_initialized_with_invalid_size)
{
    using namespace math::core::buffers;
    using namespace math::core::allocators;

    Allocated_buffer<Stack_allocator<2>> buff{4};

    EXPECT_FALSE(buff.usable());
    EXPECT_TRUE(buff.data().empty());
}

// Fallback_buffer tests

TEST(Fallback_buffer_test, uses_the_first_buffer_when_usable)
{
    using namespace math::core::buffers;
    using namespace math::core::allocators;

    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Malloc_allocator>> buff{2};

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
    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Malloc_allocator>> buff{4};

    EXPECT_TRUE(buff.usable());
    EXPECT_FALSE(buff.data().empty());
    EXPECT_NE(nullptr, buff.data().p);
    EXPECT_EQ(4, buff.data().s);
}

TEST(Allocated_buffer_test, not_usable_when_initialized_with_invalid_size_for_both_buffers)
{
    using namespace math::core::buffers;
    using namespace math::core::allocators;

    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Stack_allocator<2>>> buff{4};

    EXPECT_FALSE(buff.usable());
    EXPECT_TRUE(buff.data().empty());
}

