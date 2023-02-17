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

    // Block2
    {
        Block2<void> b{};

        EXPECT_EQ(nullptr, b.data);
        EXPECT_EQ(0, b.size);
        EXPECT_TRUE(empty(b));

        const std::uint8_t buffer[]{ 0 };
        EXPECT_FALSE(empty(Block2<const void>{ 1, buffer }));
        EXPECT_TRUE(empty(Block2<void>{ 1, nullptr }));
        EXPECT_TRUE(empty(Block2<const void>{ 0, buffer }));
    }

    Block<void> b{};

    EXPECT_EQ(nullptr, data(b));
    EXPECT_EQ(0, size(b));
    EXPECT_TRUE(empty(b));

    const std::uint8_t buffer[]{ 0 };
    EXPECT_FALSE(empty(Block<void>{ 1, buffer }));
    EXPECT_TRUE(empty(Block<void>{ 1, nullptr }));
    EXPECT_TRUE(empty(Block<void>{ 0, buffer }));
}

TEST(Block_test, can_be_of_specific_type)
{
    using namespace memoc;

    // Block2
    {
        const int values[]{ 1 };
        Block2<const int> b{ 1, values };

        EXPECT_NE(nullptr, b.data);
        EXPECT_EQ(1, b.size);
        EXPECT_FALSE(empty(b));
        EXPECT_EQ(1, b.data[0]);

        bool valid_buffer_type = std::is_same_v<const int, typename std::remove_pointer_t<decltype(b.data)>>;
        EXPECT_TRUE(valid_buffer_type);
    }

    const int values[]{ 1 };
    Block<int> b{ 1, values };

    EXPECT_NE(nullptr, data(b));
    EXPECT_EQ(1, size(b));
    EXPECT_FALSE(empty(b));
    EXPECT_EQ(1, b[0]);

    bool valid_buffer_type = std::is_same<int, typename std::remove_pointer<decltype(data(b))>::type>();
    EXPECT_TRUE(valid_buffer_type);
}

TEST(Block_test, can_be_compared_with_another_block)
{
    using namespace memoc;

    // Block2
    {
        EXPECT_EQ(Block2<int>{}, Block2<double>{});

        const int data1[]{ 1, 2, 3, 4, 5 };
        const double data2[]{ 1.0, 2.0, 3.0, 4.0, 5.1 };

        EXPECT_NE((Block2{ 5, data1 }), (Block2{ 5, data2 }));
        EXPECT_EQ((Block2{ 4, data1 }), (Block2{ 4, data2 }));

        EXPECT_NE((Block2{ 2, data1 }), (Block2{ 4, data2 }));

        // partially empty blocks considered
        EXPECT_EQ((Block2<int>{ 2, nullptr }), (Block2<double>{ 4, nullptr }));
        EXPECT_EQ((Block2{ 0, data1 }), (Block2{ 0, data2 }));

        // comparison of void type blocks
        EXPECT_EQ((Block2<const void>{ MEMOC_SSIZEOF(int) * 4, data1 }), (Block2{ 4, data1 }));
        EXPECT_NE((Block2{ 4, data2 }), (Block2<const void>{ MEMOC_SSIZEOF(int) * 4, data1 }));
    }

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

    // Block2
    {
        const int data1[]{ 1, 2, 3, 4, 5 };
        Block2<const int> sb1{ 5, data1 };

        const double data2[]{ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
        Block2<const double> sb2{ 6, data2 };

        int data3[]{ 0, 0, 0, 0, 0 };
        Block2<int> db1{ 5, data3 };

        EXPECT_EQ(4, copy(sb1, db1, 4));
        EXPECT_EQ((Block2<int>{4, db1.data}), (Block2<const int>{4, sb1.data}));

        EXPECT_EQ(5, copy(sb2, db1));
        EXPECT_EQ(db1, sb1);

        EXPECT_EQ(20, copy(Block2<const void>{MEMOC_SSIZEOF(double)* sb2.size, sb2.data}, db1));
        EXPECT_NE(db1, sb1);
        EXPECT_NE(db1, (Block2{ 5, sb2.data }));
    }

    const int data1[]{ 1, 2, 3, 4, 5 };
    Block<int> sb1{ 5, data1 };

    const double data2[]{ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
    Block<double> sb2{ 6, data2 };

    const int data3[]{ 0, 0, 0, 0, 0 };
    Block<int> db1{ 5, data3 };

    EXPECT_EQ(4, copy(sb1, db1, 4));
    EXPECT_EQ((Block<int>{4, data(db1)}), (Block<int>{4, data(sb1)}));

    EXPECT_EQ(5, copy(sb2, db1));
    EXPECT_EQ(db1, sb1);

    EXPECT_EQ(20, copy(Block<void>{MEMOC_SSIZEOF(double)* size(sb2), data(sb2)}, db1));
    EXPECT_NE(db1, sb1);
    EXPECT_NE(db1, (Block{ 5, data(sb2) }));
}

TEST(Block_test, can_be_set_by_value)
{
    using namespace memoc;

    // Block2
    {
        int values[]{ 0, 0, 0, 0, 0 };
        Block2<int> b{ 5, values };

        EXPECT_EQ(5, copy(1, b));
        for (std::int64_t i = 0; i < 5; ++i) {
            EXPECT_EQ(1, b.data[i]);
        }

        EXPECT_EQ(5, copy(0, b));
        for (std::int64_t i = 0; i < 5; ++i) {
            EXPECT_EQ(0, b.data[i]);
        }

        Block2<void> bv{ b.size * MEMOC_SSIZEOF(int), b.data };

        EXPECT_EQ(20, copy(std::uint16_t{ 0x0101 }, bv));
        for (std::int64_t i = 0; i < 5; ++i) {
            EXPECT_EQ(16843009, b.data[i]);
        }
    }

    const int values[]{ 0, 0, 0, 0, 0 };
    Block<int> b{ 5, values };

    EXPECT_EQ(5, set(b, 1));
    for (std::int64_t i = 0; i < 5; ++i) {
        EXPECT_EQ(1, data(b)[i]);
    }

    EXPECT_EQ(5, set(b, 0));
    for (std::int64_t i = 0; i < 5; ++i) {
        EXPECT_EQ(0, data(b)[i]);
    }

    Block<void> bv{ size(b) * MEMOC_SSIZEOF(int), data(b) };

    EXPECT_EQ(20, set(bv, std::uint8_t{ 1 }));
    for (std::int64_t i = 0; i < 5; ++i) {
        EXPECT_EQ(16843009, data(b)[i]);
    }
}
