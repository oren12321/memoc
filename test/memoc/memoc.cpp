#include <gtest/gtest.h>

#include <memoc/memoc.h>

TEST(Memoc_test, use_all_namespaces)
{
    using namespace memoc::allocators;
    using namespace memoc;
    using namespace memoc::buffers;
    using namespace memoc::pointers;
    SUCCEED();
}