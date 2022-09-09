#include <gtest/gtest.h>

#include <memoc/memoc.h>

TEST(Memoc_test, use_all_namespaces)
{
    using namespace memoc;
    SUCCEED();
}