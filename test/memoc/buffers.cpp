#include <gtest/gtest.h>

#include <utility>
#include <limits>
#include <stdexcept>
#include <string>

#include <memoc/buffers.h>
#include <memoc/allocators.h>
#include <memoc/blocks.h>

// Stack_buffer tests

template <memoc::Block<void>::Size_type Size>
using Test_stack_allocator = memoc::Stack_allocator<memoc::details::Default_global_stack_memory<1, Size>>;

TEST(Stack_buffer_test, not_empty_when_initialized_with_valid_size)
{
    using namespace memoc;

    Buffer<void, Test_stack_allocator<2>> buff{ 2 };

    EXPECT_FALSE(buff.empty());
    EXPECT_NE(nullptr, buff.data());
    EXPECT_EQ(2, buff.size());
}

TEST(Stack_buffer_test, throws_exception_when_initialized_with_invalid_size)
{
    using namespace memoc;

    EXPECT_THROW((Buffer<void, Test_stack_allocator<2>>{ 4 }), std::runtime_error);

    //EXPECT_TRUE(buff.empty());
}

TEST(Stack_buffer_test, DISABLED_is_copyable)
{
    using namespace memoc;

    Buffer<void, Test_stack_allocator<2>> buff1{ 2 };
    Buffer<void, Test_stack_allocator<2>> buff2{ buff1 };

    EXPECT_FALSE(buff1.empty());
    EXPECT_NE(nullptr, buff1.data());
    EXPECT_EQ(2, buff1.size());

    EXPECT_FALSE(buff2.empty());
    EXPECT_NE(nullptr, buff2.data());
    EXPECT_EQ(2, buff2.size());

    EXPECT_NE(buff1.data(), buff2.data());
    EXPECT_EQ(buff1.size(), buff2.size());

    Buffer<void, Test_stack_allocator<2>> buff3{ 2 };
    buff3 = buff2;

    EXPECT_FALSE(buff3.empty());
    EXPECT_NE(nullptr, buff3.data());
    EXPECT_EQ(2, buff3.size());

    EXPECT_NE(buff2.data(), buff3.data());
    EXPECT_EQ(buff2.size(), buff3.size());
}

TEST(Stack_buffer_test, DISABLED_is_moveable)
{
    using namespace memoc;

    Buffer<void, Test_stack_allocator<2>> buff1{ 2 };
    Buffer<void, Test_stack_allocator<2>> buff2{ std::move(buff1) };

    EXPECT_TRUE(buff1.empty());

    EXPECT_FALSE(buff2.empty());
    EXPECT_NE(nullptr, buff2.data());
    EXPECT_EQ(2, buff2.size());

    EXPECT_NE(buff1.data(), buff2.data());
    EXPECT_NE(buff1.size(), buff2.size());

    Buffer<void, Test_stack_allocator<2>> buff3{ 2 };
    buff3 = std::move(buff2);

    EXPECT_TRUE(buff2.empty());

    EXPECT_NE(buff2.data(), buff3.data());
    EXPECT_NE(buff2.size(), buff3.size());
}

TEST(Stack_buffer_test, can_be_initalized_with_data)
{
    using namespace memoc;

    const int values[2] = {1, 2};

    Buffer<int, Test_stack_allocator<2 * MEMOC_SSIZEOF(int)>> buff1{ 2, values };

    EXPECT_FALSE(buff1.empty());
    EXPECT_NE(nullptr, buff1.data());
    EXPECT_EQ(2, buff1.size());

    int* data1 = buff1.data();
    EXPECT_EQ(values[0], data1[0]);
    EXPECT_EQ(values[1], data1[1]);

    //Buffer<int, Test_stack_allocator<2 * MEMOC_SSIZEOF(int)>> copy1{buff1};

    //EXPECT_FALSE(copy1.empty());
    //EXPECT_NE(nullptr, copy1.data());
    //EXPECT_EQ(2, copy1.size());

    //EXPECT_NE(buff1.data(), copy1.data());
    //EXPECT_EQ(buff1.size(), copy1.size());

    //int* copy_data1 = copy1.data();
    //EXPECT_EQ(values[0], copy_data1[0]);
    //EXPECT_EQ(values[1], copy_data1[1]);

    //Buffer<int, Test_stack_allocator<2 * MEMOC_SSIZEOF(int)>> moved1{std::move(buff1)};

    //EXPECT_TRUE(buff1.empty());

    //EXPECT_FALSE(moved1.empty());
    //EXPECT_NE(nullptr, moved1.data());
    //EXPECT_EQ(2, moved1.size());

    //int* moved_data1 = moved1.data();
    //EXPECT_EQ(values[0], moved_data1[0]);
    //EXPECT_EQ(values[1], moved_data1[1]);
}

// Allocated_buffer tests

TEST(Allocated_buffer_test, not_empty_when_initialized_with_valid_size)
{
    using namespace memoc;

    Buffer<void, Malloc_allocator> buff{ 2 };

    EXPECT_FALSE(buff.empty());
    EXPECT_NE(nullptr, buff.data());
    EXPECT_EQ(2, buff.size());
}

TEST(Allocated_buffer_test, throws_exception_when_initialized_with_invalid_size)
{
    using namespace memoc;

    EXPECT_THROW((Buffer<void, Test_stack_allocator<2>>{ 4 }), std::runtime_error);

    //EXPECT_TRUE(buff.empty());
}

TEST(Allocated_buffer_test, is_copyable)
{
    using namespace memoc;

    Buffer<void, Malloc_allocator> buff1{ 2 };
    Buffer<void, Malloc_allocator> buff2{ buff1 };

    EXPECT_FALSE(buff1.empty());
    EXPECT_NE(nullptr, buff1.data());
    EXPECT_EQ(2, buff1.size());

    EXPECT_FALSE(buff2.empty());
    EXPECT_NE(nullptr, buff2.data());
    EXPECT_EQ(2, buff2.size());

    EXPECT_NE(buff1.data(), buff2.data());
    EXPECT_EQ(buff1.size(), buff2.size());

    Buffer<void, Malloc_allocator> buff3{ 2 };
    buff3 = buff2;

    EXPECT_FALSE(buff3.empty());
    EXPECT_NE(nullptr, buff3.data());
    EXPECT_EQ(2, buff3.size());

    EXPECT_NE(buff2.data(), buff3.data());
    EXPECT_EQ(buff2.size(), buff3.size());
}

TEST(Allocated_buffer_test, is_moveable)
{
    using namespace memoc;

    Buffer<void, Malloc_allocator> buff1{ 2 };
    Buffer<void, Malloc_allocator> buff2{ std::move(buff1) };

    EXPECT_TRUE(buff1.empty());

    EXPECT_FALSE(buff2.empty());
    EXPECT_NE(nullptr, buff2.data());
    EXPECT_EQ(2, buff2.size());

    EXPECT_NE(buff1.data(), buff2.data());
    EXPECT_NE(buff1.size(), buff2.size());

    Buffer<void, Malloc_allocator> buff3{ 2 };
    buff3 = std::move(buff2);

    EXPECT_TRUE(buff2.empty());

    EXPECT_NE(buff2.data(), buff3.data());
    EXPECT_NE(buff2.size(), buff3.size());
}

TEST(Allocated_buffer_test, can_be_initalized_with_data)
{
    using namespace memoc;

    const int values[2] = {1, 2};

    Buffer<int, Malloc_allocator> buff1{ 2, values };

    EXPECT_FALSE(buff1.empty());
    EXPECT_NE(nullptr, buff1.data());
    EXPECT_EQ(2, buff1.size());

    int* data1 = buff1.data();
    EXPECT_EQ(values[0], data1[0]);
    EXPECT_EQ(values[1], data1[1]);

    Buffer<int, Malloc_allocator> copy1{buff1};

    EXPECT_FALSE(copy1.empty());
    EXPECT_NE(nullptr, copy1.data());
    EXPECT_EQ(2, copy1.size());

    EXPECT_NE(buff1.data(), copy1.data());
    EXPECT_EQ(buff1.size(), copy1.size());

    int* copy_data1 = copy1.data();
    EXPECT_EQ(values[0], copy_data1[0]);
    EXPECT_EQ(values[1], copy_data1[1]);

    Buffer<int, Malloc_allocator> moved1{std::move(buff1)};

    EXPECT_TRUE(buff1.empty());

    EXPECT_FALSE(moved1.empty());
    EXPECT_NE(nullptr, moved1.data());
    EXPECT_EQ(2, moved1.size());

    int* moved_data1 = moved1.data();
    EXPECT_EQ(values[0], moved_data1[0]);
    EXPECT_EQ(values[1], moved_data1[1]);
}

// Fallback_buffer tests

TEST(Fallback_buffer_test, uses_the_first_buffer_when_not_empty)
{
    using namespace memoc;

    Buffer<void, Fallback_allocator<Test_stack_allocator<2>, Malloc_allocator>> buff{ 2 };

    EXPECT_FALSE(buff.empty());
    EXPECT_NE(nullptr, buff.data());
    EXPECT_EQ(2, buff.size());
}

TEST(Fallback_buffer_test, uses_the_second_buffer_when_not_empty)
{
    using namespace memoc;

    // Required size invalid for first allocator
    Buffer<void, Fallback_allocator<Test_stack_allocator<2>, Malloc_allocator>> buff{ 4 };

    EXPECT_FALSE(buff.empty());
    EXPECT_NE(nullptr, buff.data());
    EXPECT_EQ(4, buff.size());
}

TEST(Fallback_buffer_test, throws_exception_when_initialized_with_invalid_size_for_both_buffers)
{
    using namespace memoc;

    EXPECT_THROW((Buffer<void, Fallback_allocator<Test_stack_allocator<2>, Test_stack_allocator<2>>>{ 4 }), std::runtime_error);

    //EXPECT_TRUE(buff.empty());
}

// Just for compilation check
TEST(Fallback_buffer_test, DISABLED_is_copyable)
{
    using namespace memoc;

    Buffer<void, Fallback_allocator<Test_stack_allocator<2>,Test_stack_allocator<4>>> buff1{ 4 };
    Buffer<void, Fallback_allocator<Test_stack_allocator<2>,Test_stack_allocator<4>>> buff2{ buff1 };
    Buffer<void, Fallback_allocator<Test_stack_allocator<2>,Test_stack_allocator<4>>> buff3{ 4 };
    buff3 = buff2;
}

// Just for compilation check
TEST(Fallback_buffer_test, DISABLED_is_moveable)
{
    using namespace memoc;

    Buffer<void, Fallback_allocator<Test_stack_allocator<2>,Test_stack_allocator<4>>> buff1{ 4 };
    Buffer<void, Fallback_allocator<Test_stack_allocator<2>,Test_stack_allocator<4>>> buff2{ std::move(buff1) };
    Buffer<void, Fallback_allocator<Test_stack_allocator<2>,Test_stack_allocator<4>>> buff3{ 4 };
    buff3 = std::move(buff2);
}

TEST(Fallback_buffer_test, can_be_initalized_with_data)
{
    using namespace memoc;

    const int values[2] = { 1, 2 };

    Buffer<int, Fallback_allocator<Test_stack_allocator<2 * MEMOC_SSIZEOF(int)>, Malloc_allocator>> buff1{ 2, values };

    EXPECT_FALSE(buff1.empty());
    EXPECT_NE(nullptr, buff1.data());
    EXPECT_EQ(2, buff1.size());

    int* data1 = buff1.data();
    EXPECT_EQ(values[0], data1[0]);
    EXPECT_EQ(values[1], data1[1]);

    Buffer<int, Fallback_allocator<Test_stack_allocator<2 * MEMOC_SSIZEOF(int)>, Malloc_allocator>> copy1{ buff1 };

    EXPECT_FALSE(copy1.empty());
    EXPECT_NE(nullptr, copy1.data());
    EXPECT_EQ(2, copy1.size());

    int* copy_data1 = copy1.data();
    EXPECT_EQ(values[0], copy_data1[0]);
    EXPECT_EQ(values[1], copy_data1[1]);

    Buffer<int, Fallback_allocator<Test_stack_allocator<2 * MEMOC_SSIZEOF(int)>, Malloc_allocator>> moved1{ std::move(buff1) };

    EXPECT_TRUE(buff1.empty());

    EXPECT_FALSE(moved1.empty());
    EXPECT_NE(nullptr, moved1.data());
    EXPECT_EQ(2, moved1.size());

    int* moved_data1 = moved1.data();
    EXPECT_EQ(values[0], moved_data1[0]);
    EXPECT_EQ(values[1], moved_data1[1]);

    Buffer<void, Fallback_allocator<Test_stack_allocator<2>, Malloc_allocator>> buff2{ 2 * MEMOC_SSIZEOF(int), values };

    EXPECT_FALSE(buff2.empty());
    EXPECT_NE(nullptr, buff2.data());
    EXPECT_EQ(2 * MEMOC_SSIZEOF(int), buff2.size());

    int* data2 = reinterpret_cast<int*>(buff2.data());
    EXPECT_EQ(values[0], data2[0]);
    EXPECT_EQ(values[1], data2[1]);

    Buffer<void, Fallback_allocator<Test_stack_allocator<2>, Malloc_allocator>> copy2{ buff2 };

    EXPECT_FALSE(copy2.empty());
    EXPECT_NE(nullptr, copy2.data());
    EXPECT_EQ(2 * MEMOC_SSIZEOF(int), copy2.size());

    int* copy_data2 = reinterpret_cast<int*>(copy2.data());
    EXPECT_EQ(values[0], copy_data2[0]);
    EXPECT_EQ(values[1], copy_data2[1]);

    Buffer<void, Fallback_allocator<Test_stack_allocator<2>, Malloc_allocator>> moved2{ std::move(buff2) };

    EXPECT_TRUE(buff2.empty());

    EXPECT_FALSE(moved2.empty());
    EXPECT_NE(nullptr, moved2.data());
    EXPECT_EQ(2 * MEMOC_SSIZEOF(int), moved2.size());

    int* moved_data2 = reinterpret_cast<int*>(moved2.data());
    EXPECT_EQ(values[0], moved_data2[0]);
    EXPECT_EQ(values[1], moved_data2[1]);
}

// Typed_buffer tests

TEST(Typed_buffer_test, can_be_initialized_with_fundamental_data_type)
{
    using namespace memoc;

    const int values[2] = {1, 2};

    Buffer<int, Malloc_allocator> buff{2, values };

    EXPECT_FALSE(buff.empty());

    Block b = buff.block();

    EXPECT_NE(nullptr, b.data());
    EXPECT_EQ(2, b.size());

    EXPECT_EQ(values[0], b.data()[0]);
    EXPECT_EQ(values[1], b.data()[1]);
}

TEST(Typed_buffer_test, can_be_initialized_with_custom_data_type)
{
    using namespace memoc;

    struct S {
        int a = 1;
        float b = 2.2;
    };

    S s[2];
    s[0] = S{};
    s[1] = S{ 2, 4.4 };

    Buffer<S, Malloc_allocator> buff1{ 2, s };

    EXPECT_FALSE(buff1.empty());

    Block b1 = buff1.block();

    EXPECT_NE(nullptr, b1.data());
    EXPECT_EQ(2, b1.size());

    EXPECT_EQ(s[0].a, b1.data()[0].a); EXPECT_EQ(s[0].b, b1.data()[0].b);
    EXPECT_EQ(s[1].a, b1.data()[1].a); EXPECT_EQ(s[1].b, b1.data()[1].b);

    std::string data2[2]{ "first string", "second string" };

    Buffer<std::string, Test_stack_allocator<256>> buff2{ 2, data2 };

    Block b2 = buff2.block();

    EXPECT_EQ(data2[0], b2.data()[0]);
    EXPECT_EQ(data2[1], b2.data()[1]);
}

// Buffer API tests

TEST(Any_buffer_test, creation_via_create_function)
{
    using namespace memoc;

    Buffer empty_buffer = create_buffer<void>().value();
    EXPECT_TRUE(empty_buffer.empty());

    Buffer_error buffer_with_invalid_size = create_buffer<void>(-1).error();
    EXPECT_EQ(Buffer_error::invalid_size, buffer_with_invalid_size);

    // Comment in order to prevent address-sanitizer error in Linux.
    //Buffer_error buffer_with_unknown_failure = create<Buffer_type>(std::numeric_limits<std::int64_t>::max()).error();
    //EXPECT_EQ(Buffer_error::unknown, buffer_with_unknown_failure);

    Buffer buffer_with_no_data = create_buffer<void>(2).value();
    EXPECT_FALSE(buffer_with_no_data.empty());
    EXPECT_EQ(2, buffer_with_no_data.size());
    EXPECT_NE(nullptr, buffer_with_no_data.data());

    int values[]{ 150, 151 };
    Buffer<int> buffer_with_data = create_buffer<int>(2, values).value();
    EXPECT_FALSE(buffer_with_data.empty());
    EXPECT_EQ(2, buffer_with_data.size());
    EXPECT_NE(nullptr, buffer_with_data.data());
    EXPECT_EQ(150, buffer_with_data.data()[0]); EXPECT_EQ(151, buffer_with_data.data()[1]);
}
