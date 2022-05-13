#include <gtest/gtest.h>

#include <utility>

#include <math/core/pointers.h>
#include <math/core/allocators.h>
#include <math/core/memory.h>

TEST(LW_Shared_ptr, construction_and_accessors)
{
    using namespace math::core::pointers;

    struct Foo {
        Foo(int i = 0)
            : id_{ i } {}
        int id_{ 0 };
    };

    {
        // no managed object
        Shared_ptr<Foo> sp1{};
        EXPECT_FALSE(sp1);
        EXPECT_EQ(nullptr, sp1.get());
        EXPECT_EQ(0, sp1.use_count());
    }

    {
        // constructor with object
        math::core::allocators::Malloc_allocator allocator{};
        math::core::memory::Block b = allocator.allocate(sizeof(Foo));
        Foo* ptr = math::core::memory::aux::construct_at<Foo>(reinterpret_cast<Foo*>(b.p), 10);
        Shared_ptr<Foo> sp2(ptr);
        EXPECT_TRUE(sp2);
        EXPECT_NE(nullptr, sp2.get());
        EXPECT_EQ(1, sp2.use_count());
        EXPECT_EQ(10, (*sp2).id_);
        EXPECT_EQ(10, sp2->id_);
    }

    {
        // using auxiliary function
        Shared_ptr<Foo> sp3 = Shared_ptr<Foo>::make_shared(5);
        EXPECT_TRUE(sp3);
        EXPECT_NE(nullptr, sp3.get());
        EXPECT_EQ(1, sp3.use_count());
        EXPECT_EQ(5, (*sp3).id_);
        EXPECT_EQ(5, sp3->id_);
    }
}

TEST(LW_Shared_ptr, comparison)
{
    using namespace math::core::pointers;

    Shared_ptr<int> p1 = Shared_ptr<int>::make_shared(42);
    Shared_ptr<int> p2 = Shared_ptr<int>::make_shared(42);

    EXPECT_TRUE(p1 == p1);
    EXPECT_TRUE(p1 <=> p1 == 0);
    EXPECT_FALSE(p1 == p2);
    EXPECT_TRUE(p1 < p2 || p1 > p2);
    EXPECT_FALSE(p1 <=> p2 == 0);
}

TEST(LW_Shared_ptr_test, inheritance_and_sharing)
{
    using namespace math::core::pointers;

    struct A {
        A(int a)
            : a_(a) {}
        virtual ~A()
        {
            if (destructed_) {
                *destructed_ = true;
            }
        }
        int a_{ 0 };
        bool* destructed_{ nullptr };
    };

    struct B : public A {
        B(int a, int b)
            : A(a), b_(b) {}
        int b_{ 0 };
    };

    bool destructed = false;
    {
        Shared_ptr<B> sp1 = Shared_ptr<B>::make_shared(1, 2);
        EXPECT_TRUE(sp1);
        EXPECT_NE(nullptr, sp1.get());
        EXPECT_EQ(1, (*sp1).a_);
        EXPECT_EQ(1, sp1->a_);
        EXPECT_EQ(2, (*sp1).b_);
        EXPECT_EQ(2, sp1->b_);
        EXPECT_EQ(1, sp1.use_count());
        sp1->destructed_ = &destructed;
        {
            Shared_ptr<A> sp11 = sp1;
            EXPECT_TRUE(sp11);
            EXPECT_NE(nullptr, sp11.get());
            EXPECT_EQ(sp1.get(), sp11.get());
            EXPECT_EQ(1, (*sp11).a_);
            EXPECT_EQ(1, sp11->a_);
            EXPECT_EQ(2, sp11.use_count());
            EXPECT_EQ(sp1.use_count(), sp11.use_count());
        }
        EXPECT_FALSE(destructed);
        EXPECT_TRUE(sp1);
        EXPECT_NE(nullptr, sp1.get());
        EXPECT_EQ(1, (*sp1).a_);
        EXPECT_EQ(1, sp1->a_);
        EXPECT_EQ(2, (*sp1).b_);
        EXPECT_EQ(2, sp1->b_);
        EXPECT_EQ(1, sp1.use_count());
    }
    EXPECT_TRUE(destructed);
}

TEST(LW_Shared_ptr, reset)
{
    using namespace math::core::pointers;

    struct Foo {
        Foo(int i = 0)
            : id_{ i } {}
        int id_{ 0 };
    };

    {
        // unique ownership
        Shared_ptr<Foo> sp1 = Shared_ptr<Foo>::make_shared(100);
        EXPECT_EQ(1, sp1.use_count());
        EXPECT_TRUE(sp1);
        sp1.reset();
        EXPECT_EQ(0, sp1.use_count());
        EXPECT_FALSE(sp1);
    }

    {
        // multiple ownership
        Shared_ptr<Foo> sp1 = Shared_ptr<Foo>::make_shared(300);
        Shared_ptr<Foo> sp2 = sp1;
        Shared_ptr<Foo> sp3 = sp2;
        EXPECT_EQ(3, sp1.use_count());
        EXPECT_EQ(3, sp2.use_count());
        EXPECT_EQ(3, sp3.use_count());

        math::core::allocators::Malloc_allocator allocator{};
        math::core::memory::Block b = allocator.allocate(sizeof(Foo));
        Foo* ptr = math::core::memory::aux::construct_at<Foo>(reinterpret_cast<Foo*>(b.p), 10);
        sp1.reset(ptr);
        EXPECT_EQ(1, sp1.use_count());
        EXPECT_EQ(2, sp2.use_count());
        EXPECT_EQ(2, sp3.use_count());
    }
}

TEST(LW_Shared_ptr, move)
{
    using namespace math::core::pointers;

    {
        // unique ownership - constructor
        Shared_ptr<int> sp1 = Shared_ptr<int>::make_shared(100);
        EXPECT_EQ(1, sp1.use_count());
        EXPECT_TRUE(sp1);
        Shared_ptr<int> sp2{ std::move(sp1) };
        EXPECT_EQ(0, sp1.use_count());
        EXPECT_FALSE(sp1);
        EXPECT_EQ(1, sp2.use_count());
        EXPECT_TRUE(sp2);
    }

    {
        // unique ownership - assignment
        Shared_ptr<int> sp1 = Shared_ptr<int>::make_shared(100);
        EXPECT_EQ(1, sp1.use_count());
        EXPECT_TRUE(sp1);
        Shared_ptr<int> sp2{};
        sp2 = std::move(sp1);
        EXPECT_EQ(0, sp1.use_count());
        EXPECT_FALSE(sp1);
        EXPECT_EQ(1, sp2.use_count());
        EXPECT_TRUE(sp2);
    }

    {
        // multiple ownership - constructor
        Shared_ptr<int> sp1 = Shared_ptr<int>::make_shared(100);
        EXPECT_EQ(1, sp1.use_count());
        EXPECT_TRUE(sp1);
        Shared_ptr<int> sp2 = sp1;
        EXPECT_EQ(2, sp1.use_count());
        EXPECT_TRUE(sp1);
        EXPECT_EQ(2, sp2.use_count());
        EXPECT_TRUE(sp2);
        Shared_ptr<int> sp3{ std::move(sp2) };
        EXPECT_EQ(2, sp1.use_count());
        EXPECT_TRUE(sp1);
        EXPECT_EQ(0, sp2.use_count());
        EXPECT_FALSE(sp2);
        EXPECT_EQ(2, sp3.use_count());
        EXPECT_TRUE(sp3);
    }

    {
        // multiple ownership - constructor
        Shared_ptr<int> sp1 = Shared_ptr<int>::make_shared(100);
        EXPECT_EQ(1, sp1.use_count());
        EXPECT_TRUE(sp1);
        Shared_ptr<int> sp2 = sp1;
        EXPECT_EQ(2, sp1.use_count());
        EXPECT_TRUE(sp1);
        EXPECT_EQ(2, sp2.use_count());
        EXPECT_TRUE(sp2);
        Shared_ptr<int> sp3{};
        sp3 = std::move(sp2);
        EXPECT_EQ(2, sp1.use_count());
        EXPECT_TRUE(sp1);
        EXPECT_EQ(0, sp2.use_count());
        EXPECT_FALSE(sp2);
        EXPECT_EQ(2, sp3.use_count());
        EXPECT_TRUE(sp3);
    }
}
