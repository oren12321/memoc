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

    Block b{};

    EXPECT_EQ(nullptr, b.p());
    EXPECT_EQ(0, b.s());
    EXPECT_TRUE(b.empty());

    const std::uint8_t buffer[]{ 0 };
    EXPECT_FALSE((Block{ 1, buffer }).empty());
    EXPECT_TRUE((Block{ 1, nullptr }).empty());
    EXPECT_TRUE((Block{ 0, buffer }).empty());
}

TEST(Block_test, can_be_of_specific_type)
{
    using namespace memoc;

    Typed_block<int> b{};

    EXPECT_EQ(nullptr, b.p());
    EXPECT_EQ(0, b.s());
    EXPECT_TRUE(b.empty());

    bool valid_buffer_type = std::is_same<int, typename std::remove_pointer<decltype(b.p())>::type>();
    EXPECT_TRUE(valid_buffer_type);
}

TEST(Block_test, can_be_compared_with_another_block_of_the_same_type)
{
    using namespace memoc;

    EXPECT_EQ(Typed_block<int>{}, Typed_block<double>{});

    const int data1[]{ 1, 2, 3, 4, 5 };
    const double data2[]{ 1.0, 2.0, 3.0, 4.0, 5.1 };

    EXPECT_NE((Typed_block{ 5, data1 }), (Typed_block{ 5, data2 }));
    EXPECT_EQ((Typed_block{ 4, data1 }), (Typed_block{ 4, data2 }));

    EXPECT_NE((Typed_block{ 2, data1 }), (Typed_block{ 4, data2 }));

    // partially empty blocks considered
    EXPECT_EQ((Typed_block<int>{ 2, nullptr }), (Typed_block<double>{ 4, nullptr }));
    EXPECT_EQ((Typed_block{ 0, data1 }), (Typed_block{ 0, data2 }));
}
