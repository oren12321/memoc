#include <gtest/gtest.h>

#include <memoc/blocks.h>

TEST(Block_test, is_empty_when_deafult_initalized)
{
    using namespace memoc;

    Block b{};

    EXPECT_EQ(nullptr, b.p);
    EXPECT_EQ(0, b.s);
    EXPECT_TRUE(b.empty());
}

TEST(Block_test, can_be_of_specific_type)
{
    using namespace memoc;

    Typed_block<int> b{};

    EXPECT_EQ(nullptr, b.p);
    EXPECT_EQ(0, b.s);
    EXPECT_TRUE(b.empty());

    bool valid_buffer_type = std::is_same<int, typename std::remove_pointer<decltype(b.p)>::type>();
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
    EXPECT_NE((Typed_block<int>{ 2, nullptr }), (Typed_block<double>{ 4, nullptr }));
    EXPECT_NE((Typed_block{ 0, data1 }), (Typed_block{ 0, data2 }));
}
