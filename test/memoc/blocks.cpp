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

    const int data[]{ 1 };
    Block<int> b{ 1, data };

    EXPECT_NE(nullptr, b.p());
    EXPECT_EQ(1, b.s());
    EXPECT_FALSE(b.empty());
    EXPECT_EQ(1, b[0]);

    bool valid_buffer_type = std::is_same<int, typename std::remove_pointer<decltype(b.p())>::type>();
    EXPECT_TRUE(valid_buffer_type);
}

TEST(Block_test, can_be_compared_with_another_block)
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

    // comparison of void type blocks
    EXPECT_EQ((Block<void>{ MEMOC_SSIZEOF(int) * 4, data1 }), (Block{ 4, data1 }));
    EXPECT_NE((Block{ 4, data2 }), (Block<void>{ MEMOC_SSIZEOF(int) * 4, data1 }));
}

TEST(Block_test, can_be_copied_to_another_block)
{
    using namespace memoc;

    const int data1[]{ 1, 2, 3, 4, 5 };
    Block<int> sb1{ 5, data1 };

    const double data2[]{ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
    Block<double> sb2{ 6, data2 };

    const int data3[]{ 0, 0, 0, 0, 0 };
    Block<int> db1{ 5, data3 };

    EXPECT_EQ(4, copy(sb1, db1, 4));
    EXPECT_EQ((Block<int>{4, db1.p()}), (Block<int>{4, sb1.p()}));

    EXPECT_EQ(5, copy(sb2, db1));
    EXPECT_EQ(db1, sb1);

    EXPECT_EQ(20, copy(Block<void>{MEMOC_SSIZEOF(double)* sb2.s(), sb2.p()}, db1));
    EXPECT_NE(db1, sb1);
    EXPECT_NE(db1, (Block{ 5, sb2.p() }));
}

TEST(Block_test, can_be_set_by_value)
{
    using namespace memoc;

    const int data[]{ 0, 0, 0, 0, 0 };
    Block<int> b{ 5, data };

    EXPECT_EQ(5, set(b, 1));
    for (std::int64_t i = 0; i < 5; ++i) {
        EXPECT_EQ(1, b.p()[i]);
    }

    EXPECT_EQ(5, set(b, 0));
    for (std::int64_t i = 0; i < 5; ++i) {
        EXPECT_EQ(0, b.p()[i]);
    }

    Block<void> bv{ b.s() * MEMOC_SSIZEOF(int), b.p() };

    EXPECT_EQ(20, set(bv, std::uint8_t{ 1 }));
    for (std::int64_t i = 0; i < 5; ++i) {
        EXPECT_EQ(16843009, b.p()[i]);
    }
}
