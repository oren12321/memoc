#include <gtest/gtest.h>

#include <memoc/blocks.h>

TEST(Block_test, is_empty_when_deafult_initalized)
{
    using namespace memoc::blocks;

    Block b{};

    EXPECT_EQ(nullptr, b.p);
    EXPECT_EQ(0, b.s);
    EXPECT_TRUE(b.empty());
}

TEST(Block_test, can_be_of_specific_type)
{
    using namespace memoc::blocks;

    Typed_block<int> b{};

    EXPECT_EQ(nullptr, b.p);
    EXPECT_EQ(0, b.s);
    EXPECT_TRUE(b.empty());

    bool valid_buffer_type = std::is_same<int, typename std::remove_pointer<decltype(b.p)>::type>();
    EXPECT_TRUE(valid_buffer_type);
}
