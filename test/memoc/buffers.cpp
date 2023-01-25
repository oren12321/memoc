#include <gtest/gtest.h>

#include <utility>
#include <limits>
#include <stdexcept>

#include <memoc/buffers.h>
#include <memoc/allocators.h>
#include <memoc/blocks.h>

// Stack_buffer tests

TEST(Stack_buffer_test, not_empty_when_initialized_with_valid_size)
{
    using namespace memoc;

    Stack_buffer buff{ 2 };

    EXPECT_FALSE(empty(buff));
    EXPECT_NE(nullptr, data(buff));
    EXPECT_EQ(2, size(buff));
}

TEST(Stack_buffer_test, throws_exception_when_initialized_with_invalid_size)
{
    using namespace memoc;

    EXPECT_THROW(Stack_buffer{ 4 }, std::invalid_argument);

    //EXPECT_TRUE(empty(buff));
}

TEST(Stack_buffer_test, is_copyable)
{
    using namespace memoc;

    Stack_buffer buff1{ 2 };
    Stack_buffer buff2{ buff1 };

    EXPECT_FALSE(empty(buff1));
    EXPECT_NE(nullptr, data(buff1));
    EXPECT_EQ(2, size(buff1));

    EXPECT_FALSE(empty(buff2));
    EXPECT_NE(nullptr, data(buff2));
    EXPECT_EQ(2, size(buff2));

    EXPECT_NE(data(buff1), data(buff2));
    EXPECT_EQ(size(buff1), size(buff2));

    Stack_buffer buff3{ 2 };
    buff3 = buff2;

    EXPECT_FALSE(empty(buff3));
    EXPECT_NE(nullptr, data(buff3));
    EXPECT_EQ(2, size(buff3));

    EXPECT_NE(data(buff2), data(buff3));
    EXPECT_EQ(size(buff2), size(buff3));
}

TEST(Stack_buffer_test, is_moveable)
{
    using namespace memoc;

    Stack_buffer buff1{ 2 };
    Stack_buffer buff2{ std::move(buff1) };

    EXPECT_TRUE(empty(buff1));

    EXPECT_FALSE(empty(buff2));
    EXPECT_NE(nullptr, data(buff2));
    EXPECT_EQ(2, size(buff2));

    EXPECT_NE(data(buff1), data(buff2));
    EXPECT_NE(size(buff1), size(buff2));

    Stack_buffer buff3{ 2 };
    buff3 = std::move(buff2);

    EXPECT_TRUE(empty(buff2));

    EXPECT_NE(data(buff2), data(buff3));
    EXPECT_NE(size(buff2), size(buff3));
}

TEST(Stack_buffer_test, can_be_initalized_with_data)
{
    using namespace memoc;

    const int values[2] = {1, 2};

    Stack_buffer<2 * MEMOC_SSIZEOF(int)> buff1{ 2 * MEMOC_SSIZEOF(int), values };

    EXPECT_FALSE(empty(buff1));
    EXPECT_NE(nullptr, data(buff1));
    EXPECT_EQ(2 * MEMOC_SSIZEOF(int), size(buff1));

    int* data1 = reinterpret_cast<int*>(data(buff1));
    EXPECT_EQ(values[0], data1[0]);
    EXPECT_EQ(values[1], data1[1]);

    Stack_buffer<2 * MEMOC_SSIZEOF(int)> copy1{buff1};

    EXPECT_FALSE(empty(copy1));
    EXPECT_NE(nullptr, data(copy1));
    EXPECT_EQ(2 * MEMOC_SSIZEOF(int), size(copy1));

    EXPECT_NE(data(buff1), data(copy1));
    EXPECT_EQ(size(buff1), size(copy1));

    int* copy_data1 = reinterpret_cast<int*>(data(copy1));
    EXPECT_EQ(values[0], copy_data1[0]);
    EXPECT_EQ(values[1], copy_data1[1]);

    Stack_buffer<2 * MEMOC_SSIZEOF(int)> moved1{std::move(buff1)};

    EXPECT_TRUE(empty(buff1));

    EXPECT_FALSE(empty(moved1));
    EXPECT_NE(nullptr, data(moved1));
    EXPECT_EQ(2 * MEMOC_SSIZEOF(int), size(moved1));

    int* moved_data1 = reinterpret_cast<int*>(data(moved1));
    EXPECT_EQ(values[0], moved_data1[0]);
    EXPECT_EQ(values[1], moved_data1[1]);
}

// Allocated_buffer tests

TEST(Allocated_buffer_test, not_empty_when_initialized_with_valid_size)
{
    using namespace memoc;

    Allocated_buffer<Malloc_allocator> buff{ 2 };

    EXPECT_FALSE(empty(buff));
    EXPECT_NE(nullptr, data(buff));
    EXPECT_EQ(2, size(buff));
}

TEST(Allocated_buffer_test, throws_exception_when_initialized_with_invalid_size)
{
    using namespace memoc;

    EXPECT_THROW(Allocated_buffer<Stack_allocator<2>>{ 4 }, std::runtime_error);

    //EXPECT_TRUE(empty(buff));
}

TEST(Allocated_buffer_test, is_copyable)
{
    using namespace memoc;

    Allocated_buffer<Malloc_allocator> buff1{ 2 };
    Allocated_buffer<Malloc_allocator> buff2{ buff1 };

    EXPECT_FALSE(empty(buff1));
    EXPECT_NE(nullptr, data(buff1));
    EXPECT_EQ(2, size(buff1));

    EXPECT_FALSE(empty(buff2));
    EXPECT_NE(nullptr, data(buff2));
    EXPECT_EQ(2, size(buff2));

    EXPECT_NE(data(buff1), data(buff2));
    EXPECT_EQ(size(buff1), size(buff2));

    Allocated_buffer<Malloc_allocator> buff3{ 2 };
    buff3 = buff2;

    EXPECT_FALSE(empty(buff3));
    EXPECT_NE(nullptr, data(buff3));
    EXPECT_EQ(2, size(buff3));

    EXPECT_NE(data(buff2), data(buff3));
    EXPECT_EQ(size(buff2), size(buff3));
}

TEST(Allocated_buffer_test, is_moveable)
{
    using namespace memoc;

    Allocated_buffer<Malloc_allocator> buff1{ 2 };
    Allocated_buffer<Malloc_allocator> buff2{ std::move(buff1) };

    EXPECT_TRUE(empty(buff1));

    EXPECT_FALSE(empty(buff2));
    EXPECT_NE(nullptr, data(buff2));
    EXPECT_EQ(2, size(buff2));

    EXPECT_NE(data(buff1), data(buff2));
    EXPECT_NE(size(buff1), size(buff2));

    Allocated_buffer<Malloc_allocator> buff3{ 2 };
    buff3 = std::move(buff2);

    EXPECT_TRUE(empty(buff2));

    EXPECT_NE(data(buff2), data(buff3));
    EXPECT_NE(size(buff2), size(buff3));
}

TEST(Allocated_buffer_test, can_be_initalized_with_data)
{
    using namespace memoc;

    const int values[2] = {1, 2};

    Allocated_buffer<Malloc_allocator> buff1{ 2 * MEMOC_SSIZEOF(int), values };

    EXPECT_FALSE(empty(buff1));
    EXPECT_NE(nullptr, data(buff1));
    EXPECT_EQ(2 * MEMOC_SSIZEOF(int), size(buff1));

    int* data1 = reinterpret_cast<int*>(data(buff1));
    EXPECT_EQ(values[0], data1[0]);
    EXPECT_EQ(values[1], data1[1]);

    Allocated_buffer<Malloc_allocator> copy1{buff1};

    EXPECT_FALSE(empty(copy1));
    EXPECT_NE(nullptr, data(copy1));
    EXPECT_EQ(2 * MEMOC_SSIZEOF(int), size(copy1));

    EXPECT_NE(data(buff1), data(copy1));
    EXPECT_EQ(size(buff1), size(copy1));

    int* copy_data1 = reinterpret_cast<int*>(data(copy1));
    EXPECT_EQ(values[0], copy_data1[0]);
    EXPECT_EQ(values[1], copy_data1[1]);

    Allocated_buffer<Malloc_allocator> moved1{std::move(buff1)};

    EXPECT_TRUE(empty(buff1));

    EXPECT_FALSE(empty(moved1));
    EXPECT_NE(nullptr, data(moved1));
    EXPECT_EQ(2 * MEMOC_SSIZEOF(int), size(moved1));

    int* moved_data1 = reinterpret_cast<int*>(data(moved1));
    EXPECT_EQ(values[0], moved_data1[0]);
    EXPECT_EQ(values[1], moved_data1[1]);
}

// Fallback_buffer tests

TEST(Fallback_buffer_test, uses_the_first_buffer_when_not_empty)
{
    using namespace memoc;

    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Malloc_allocator>> buff{ 2 };

    EXPECT_FALSE(empty(buff));
    EXPECT_NE(nullptr, data(buff));
    EXPECT_EQ(2, size(buff));
}

TEST(Fallback_buffer_test, uses_the_second_buffer_when_not_empty)
{
    using namespace memoc;

    // Required size invalid for first allocator
    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Malloc_allocator>> buff{ 4 };

    EXPECT_FALSE(empty(buff));
    EXPECT_NE(nullptr, data(buff));
    EXPECT_EQ(4, size(buff));
}

TEST(Fallback_buffer_test, throws_exception_when_initialized_with_invalid_size_for_both_buffers)
{
    using namespace memoc;

    EXPECT_THROW((Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Stack_allocator<2>>>{ 4 }), std::runtime_error);

    //EXPECT_TRUE(empty(buff));
}

// Just for compilation check
TEST(Fallback_buffer_test, is_copyable)
{
    using namespace memoc;

    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Stack_allocator<4>>> buff1{ 4 };
    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Stack_allocator<4>>> buff2{ buff1 };
    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Stack_allocator<4>>> buff3{ 4 };
    buff3 = buff2;
}

// Just for compilation check
TEST(Fallback_buffer_test, is_moveable)
{
    using namespace memoc;

    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Stack_allocator<4>>> buff1{ 4 };
    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Stack_allocator<4>>> buff2{ std::move(buff1) };
    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Stack_allocator<4>>> buff3{ 4 };
    buff3 = std::move(buff2);
}

TEST(Fallback_buffer_test, can_be_initalized_with_data)
{
    using namespace memoc;

    const int values[2] = { 1, 2 };

    Fallback_buffer<Stack_buffer<2 * MEMOC_SSIZEOF(int)>, Allocated_buffer<Malloc_allocator>> buff1{ 2 * MEMOC_SSIZEOF(int), values };

    EXPECT_FALSE(empty(buff1));
    EXPECT_NE(nullptr, data(buff1));
    EXPECT_EQ(2 * MEMOC_SSIZEOF(int), size(buff1));

    int* data1 = reinterpret_cast<int*>(data(buff1));
    EXPECT_EQ(values[0], data1[0]);
    EXPECT_EQ(values[1], data1[1]);

    Fallback_buffer<Stack_buffer<2 * MEMOC_SSIZEOF(int)>, Allocated_buffer<Malloc_allocator>> copy1{ buff1 };

    EXPECT_FALSE(empty(copy1));
    EXPECT_NE(nullptr, data(copy1));
    EXPECT_EQ(2 * MEMOC_SSIZEOF(int), size(copy1));

    int* copy_data1 = reinterpret_cast<int*>(data(copy1));
    EXPECT_EQ(values[0], copy_data1[0]);
    EXPECT_EQ(values[1], copy_data1[1]);

    Fallback_buffer<Stack_buffer<2 * MEMOC_SSIZEOF(int)>, Allocated_buffer<Malloc_allocator>> moved1{ std::move(buff1) };

    EXPECT_TRUE(empty(buff1));

    EXPECT_FALSE(empty(moved1));
    EXPECT_NE(nullptr, data(moved1));
    EXPECT_EQ(2 * MEMOC_SSIZEOF(int), size(moved1));

    int* moved_data1 = reinterpret_cast<int*>(data(moved1));
    EXPECT_EQ(values[0], moved_data1[0]);
    EXPECT_EQ(values[1], moved_data1[1]);

    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Malloc_allocator>> buff2{ 2 * MEMOC_SSIZEOF(int), values };

    EXPECT_FALSE(empty(buff2));
    EXPECT_NE(nullptr, data(buff2));
    EXPECT_EQ(2 * MEMOC_SSIZEOF(int), size(buff2));

    int* data2 = reinterpret_cast<int*>(data(buff2));
    EXPECT_EQ(values[0], data2[0]);
    EXPECT_EQ(values[1], data2[1]);

    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Malloc_allocator>> copy2{ buff2 };

    EXPECT_FALSE(empty(copy2));
    EXPECT_NE(nullptr, data(copy2));
    EXPECT_EQ(2 * MEMOC_SSIZEOF(int), size(copy2));

    int* copy_data2 = reinterpret_cast<int*>(data(copy2));
    EXPECT_EQ(values[0], copy_data2[0]);
    EXPECT_EQ(values[1], copy_data2[1]);

    Fallback_buffer<Stack_buffer<2>, Allocated_buffer<Malloc_allocator>> moved2{ std::move(buff2) };

    EXPECT_TRUE(empty(buff2));

    EXPECT_FALSE(empty(moved2));
    EXPECT_NE(nullptr, data(moved2));
    EXPECT_EQ(2 * MEMOC_SSIZEOF(int), size(moved2));

    int* moved_data2 = reinterpret_cast<int*>(data(moved2));
    EXPECT_EQ(values[0], moved_data2[0]);
    EXPECT_EQ(values[1], moved_data2[1]);
}

// Typed_buffer tests

TEST(Typed_buffer_test, can_be_initialized_with_fundamental_data_type)
{
    using namespace memoc;

    const int values[2] = {1, 2};

    Typed_buffer<int, Allocated_buffer<Malloc_allocator>> buff{2, values };

    EXPECT_FALSE(empty(buff));

    Block b = block(buff);

    EXPECT_NE(nullptr, data(b));
    EXPECT_EQ(2, size(b));

    EXPECT_EQ(values[0], data(b)[0]);
    EXPECT_EQ(values[1], data(b)[1]);
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

    Typed_buffer<S, Allocated_buffer<Malloc_allocator>> buff{ 2, s };

    EXPECT_FALSE(empty(buff));

    Block b = block(buff);

    EXPECT_NE(nullptr, data(b));
    EXPECT_EQ(2, size(b));

    EXPECT_EQ(s[0].a, data(b)[0].a); EXPECT_EQ(s[0].b, data(b)[0].b);
    EXPECT_EQ(s[1].a, data(b)[1].a); EXPECT_EQ(s[1].b, data(b)[1].b);
}

// Buffer API tests

TEST(Any_buffer_test, creation_via_create_function)
{
    using namespace memoc;

    using Buffer_type = Allocated_buffer<Malloc_allocator>;

    Buffer_type empty_buffer = create<Buffer_type>().value();
    EXPECT_TRUE(empty(empty_buffer));

    Buffer_error buffer_with_invalid_size = create<Buffer_type>(-1).error();
    EXPECT_EQ(Buffer_error::invalid_size, buffer_with_invalid_size);

    // Comment in order to prevent address-sanitizer error in Linux.
    //Buffer_error buffer_with_unknown_failure = create<Buffer_type>(std::numeric_limits<std::int64_t>::max()).error();
    //EXPECT_EQ(Buffer_error::unknown, buffer_with_unknown_failure);

    Buffer_type buffer_with_no_data = create<Buffer_type>(2).value();
    EXPECT_FALSE(empty(buffer_with_no_data));
    EXPECT_EQ(2, size(buffer_with_no_data));
    EXPECT_NE(nullptr, data(buffer_with_no_data));

    int values[]{ 150, 151 };
    Typed_buffer<int, Buffer_type> buffer_with_data = create<Typed_buffer<int, Buffer_type>>(2, values).value();
    EXPECT_FALSE(empty(buffer_with_data));
    EXPECT_EQ(2, size(buffer_with_data));
    EXPECT_NE(nullptr, data(buffer_with_data));
    EXPECT_EQ(150, data(buffer_with_data)[0]); EXPECT_EQ(151, data(buffer_with_data)[1]);
}
