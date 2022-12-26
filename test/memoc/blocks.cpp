#include <gtest/gtest.h>

#include <cstdint>

#include <memoc/blocks.h>

TEST(Ssize_of_test, signed_version_of_sizeof)
{
    struct S {
        char buff[128];
    };
    EXPECT_EQ(sizeof(S), MEMOC_SSIZEOF(S));
}

TEST(Block_test, is_empty_when_deafult_initalized_or_when_initialized_partially_empty)
{
    using namespace memoc;

    Block<void> b{};

    EXPECT_EQ(nullptr, b.p());
    EXPECT_EQ(0, b.s());
    EXPECT_TRUE(b.empty());

    const std::uint8_t buffer[]{ 0 };
    EXPECT_FALSE((Block<void>{ 1, buffer }).empty());
    EXPECT_TRUE((Block<void>{ 1, nullptr }).empty());
    EXPECT_TRUE((Block<void>{ 0, buffer }).empty());
}

TEST(Block_test, can_be_of_specific_type)
{
    using namespace memoc;

    Block<int> b{};

    EXPECT_EQ(nullptr, b.p());
    EXPECT_EQ(0, b.s());
    EXPECT_TRUE(b.empty());

    bool valid_buffer_type = std::is_same<int, typename std::remove_pointer<decltype(b.p())>::type>();
    EXPECT_TRUE(valid_buffer_type);
}

TEST(Block_test, can_be_compared_with_another_block_of_the_same_type)
{
    using namespace memoc;

    EXPECT_EQ(Block<int>{}, Block<double>{});

    const int data1[]{ 1, 2, 3, 4, 5 };
    const double data2[]{ 1.0, 2.0, 3.0, 4.0, 5.1 };

    EXPECT_NE((Block{ 5, data1 }), (Block{ 5, data2 }));
    EXPECT_EQ((Block{ 4, data1 }), (Block{ 4, data2 }));

    EXPECT_NE((Block{ 2, data1 }), (Block{ 4, data2 }));

    // partially empty blocks considered
    EXPECT_EQ((Block<int>{ 2, nullptr }), (Block<double>{ 4, nullptr }));
    EXPECT_EQ((Block{ 0, data1 }), (Block{ 0, data2 }));
}
