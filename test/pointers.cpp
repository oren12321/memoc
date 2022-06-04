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
        Shared_ptr<Foo> sp3 = make_shared<Foo>(5);
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

    Shared_ptr<int> p1 = make_shared<int>(42);
    Shared_ptr<int> p2 = make_shared<int>(42);

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
        Shared_ptr<B> sp1 = make_shared<B>(1, 2);
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
        Shared_ptr<Foo> sp1 = make_shared<Foo>(100);
        EXPECT_EQ(1, sp1.use_count());
        EXPECT_TRUE(sp1);
        sp1.reset();
        EXPECT_EQ(0, sp1.use_count());
        EXPECT_FALSE(sp1);
    }

    {
        // multiple ownership
        Shared_ptr<Foo> sp1 = make_shared<Foo>(300);
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

TEST(LW_Shared_ptr, copy)
{
    using namespace math::core::pointers;

    {
        // unique ownership - constructor
        Shared_ptr<int> sp1 = make_shared<int>(100);
        EXPECT_EQ(1, sp1.use_count());
        EXPECT_TRUE(sp1);
        Shared_ptr<int> sp2{ sp1 };
        EXPECT_EQ(2, sp1.use_count());
        EXPECT_TRUE(sp1);
        EXPECT_EQ(2, sp2.use_count());
        EXPECT_TRUE(sp2);
    }

    {
        // unique ownership - assignment
        Shared_ptr<int> sp1 = make_shared<int>(100);
        EXPECT_EQ(1, sp1.use_count());
        EXPECT_TRUE(sp1);
        Shared_ptr<int> sp2{};
        sp2 = sp1;
        EXPECT_EQ(2, sp1.use_count());
        EXPECT_TRUE(sp1);
        EXPECT_EQ(2, sp2.use_count());
        EXPECT_TRUE(sp2);
    }

    {
        // multiple ownership - constructor
        Shared_ptr<int> sp1 = make_shared<int>(100);
        EXPECT_EQ(1, sp1.use_count());
        EXPECT_TRUE(sp1);
        Shared_ptr<int> sp2 = sp1;
        EXPECT_EQ(2, sp1.use_count());
        EXPECT_TRUE(sp1);
        EXPECT_EQ(2, sp2.use_count());
        EXPECT_TRUE(sp2);
        Shared_ptr<int> sp3{ sp2 };
        EXPECT_EQ(3, sp1.use_count());
        EXPECT_TRUE(sp1);
        EXPECT_EQ(3, sp2.use_count());
        EXPECT_TRUE(sp2);
        EXPECT_EQ(3, sp3.use_count());
        EXPECT_TRUE(sp3);
    }

    {
        // multiple ownership - assignment
        Shared_ptr<int> sp1 = make_shared<int>(100);
        EXPECT_EQ(1, sp1.use_count());
        EXPECT_TRUE(sp1);
        Shared_ptr<int> sp2 = sp1;
        EXPECT_EQ(2, sp1.use_count());
        EXPECT_TRUE(sp1);
        EXPECT_EQ(2, sp2.use_count());
        EXPECT_TRUE(sp2);
        Shared_ptr<int> sp3{};
        sp3 = sp2;
        EXPECT_EQ(3, sp1.use_count());
        EXPECT_TRUE(sp1);
        EXPECT_EQ(3, sp2.use_count());
        EXPECT_TRUE(sp2);
        EXPECT_EQ(3, sp3.use_count());
        EXPECT_TRUE(sp3);
    }

    {
        // aliasing
        Shared_ptr<int> sp1 = make_shared<int>(100);
        EXPECT_EQ(1, sp1.use_count());
        EXPECT_TRUE(sp1);
        Shared_ptr<int> sp2 = sp1;
        EXPECT_EQ(2, sp1.use_count());
        EXPECT_TRUE(sp1);
        EXPECT_EQ(2, sp2.use_count());
        EXPECT_TRUE(sp2);
        Shared_ptr<int> sp3{ sp2, new int{200} };
        EXPECT_EQ(3, sp1.use_count());
        EXPECT_TRUE(sp1);
        EXPECT_EQ(100, *sp1);
        EXPECT_EQ(3, sp2.use_count());
        EXPECT_TRUE(sp2);
        EXPECT_EQ(100, *sp2);
        EXPECT_EQ(3, sp3.use_count());
        EXPECT_TRUE(sp3);
        EXPECT_EQ(200, *sp3);
    }
}

TEST(LW_Shared_ptr, move)
{
    using namespace math::core::pointers;

    {
        // unique ownership - constructor
        Shared_ptr<int> sp1 = make_shared<int>(100);
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
        Shared_ptr<int> sp1 = make_shared<int>(100);
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
        Shared_ptr<int> sp1 = make_shared<int>(100);
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
        // multiple ownership - assignment
        Shared_ptr<int> sp1 = make_shared<int>(100);
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

    {
        // aliasing
        Shared_ptr<int> sp1 = make_shared<int>(100);
        EXPECT_EQ(1, sp1.use_count());
        EXPECT_TRUE(sp1);
        Shared_ptr<int> sp2 = sp1;
        EXPECT_EQ(2, sp1.use_count());
        EXPECT_TRUE(sp1);
        EXPECT_EQ(2, sp2.use_count());
        EXPECT_TRUE(sp2);
        Shared_ptr<int> sp3{ std::move(sp2), new int{200} };
        EXPECT_EQ(2, sp1.use_count());
        EXPECT_TRUE(sp1);
        EXPECT_EQ(100, *sp1);
        EXPECT_EQ(0, sp2.use_count());
        EXPECT_FALSE(sp2);
        EXPECT_EQ(2, sp3.use_count());
        EXPECT_TRUE(sp3);
        EXPECT_EQ(200, *sp3);
    }
}

TEST(LW_Shared_ptr, casting)
{
    using namespace math::core::pointers;
    struct A {
        A(int a)
            : a_(a) {}
        virtual ~A() {}
        int a_{ 0 };
    };

    struct B : public A {
        B(int a, int b)
            : A(a), b_(b) {}
        virtual ~B() {}
        int b_{ 0 };
    };

    struct C : public A {
        C(int a, int c)
            : A(a), c_(c) {}
        virtual ~C() {}
        int c_{ 0 };
    };

    struct D {};

    // static cast
    {
        Shared_ptr<B> b1 = make_shared<B>(1, 2);
        EXPECT_EQ(1, b1.use_count());
        EXPECT_TRUE(b1);
        EXPECT_EQ(1, b1->a_);
        EXPECT_EQ(2, b1->b_);

        Shared_ptr<A> a1 = static_pointer_cast<A>(b1);
        EXPECT_EQ(2, b1.use_count());
        EXPECT_TRUE(b1);
        EXPECT_EQ(2, a1.use_count());
        EXPECT_EQ(1, a1->a_);

        Shared_ptr<B> b2 = static_pointer_cast<B>(a1);
        EXPECT_EQ(3, b2.use_count());
        EXPECT_TRUE(b2);

        Shared_ptr<A> a2 = static_pointer_cast<A>(std::move(b1));
        EXPECT_EQ(0, b1.use_count());
        EXPECT_FALSE(b1);
        EXPECT_EQ(3, a2.use_count());
        EXPECT_EQ(1, a2->a_);
    }

    // dynamic cast
    {
        Shared_ptr<B> b1 = make_shared<B>(1, 2);
        EXPECT_EQ(1, b1.use_count());
        EXPECT_TRUE(b1);
        EXPECT_EQ(1, b1->a_);
        EXPECT_EQ(2, b1->b_);

        Shared_ptr<A> a1 = dynamic_pointer_cast<A>(b1);
        EXPECT_EQ(2, b1.use_count());
        EXPECT_TRUE(b1);
        EXPECT_EQ(2, a1.use_count());
        EXPECT_EQ(1, a1->a_);

        Shared_ptr<A> c = make_shared<C>(1, 2);
        Shared_ptr<B> a2 = dynamic_pointer_cast<B>(c);
        EXPECT_EQ(0, a2.use_count());
        EXPECT_FALSE(a2);

        Shared_ptr<A> a3 = dynamic_pointer_cast<A>(std::move(b1));
        EXPECT_EQ(0, b1.use_count());
        EXPECT_FALSE(b1);
        EXPECT_EQ(2, a3.use_count());
        EXPECT_EQ(1, a3->a_);
    }

    // const cast
    {
        Shared_ptr<int> a1 = make_shared<int>(1);
        EXPECT_EQ(1, a1.use_count());
        EXPECT_TRUE(a1);
        EXPECT_EQ(1, *a1);

        Shared_ptr<const int> a2 = const_pointer_cast<const int>(a1);
        *a1 = 2;
        EXPECT_EQ(2, a1.use_count());
        EXPECT_TRUE(a1);
        EXPECT_EQ(2, *a1);
        EXPECT_EQ(2, a2.use_count());
        EXPECT_TRUE(a2);
        EXPECT_EQ(2, *a2);

        Shared_ptr<const int> a3 = const_pointer_cast<const int>(std::move(a1));
        EXPECT_EQ(0, a1.use_count());
        EXPECT_FALSE(a1);
        EXPECT_EQ(2, a3.use_count());
        EXPECT_TRUE(a3);
        EXPECT_EQ(2, *a3);
    }

    // reinterpret cast
    {
        Shared_ptr<int> a1 = make_shared<int>(1);
        EXPECT_EQ(1, a1.use_count());
        EXPECT_TRUE(a1);
        EXPECT_EQ(1, *a1);

        Shared_ptr<D> a2 = reinterpret_pointer_cast<D>(a1);
        EXPECT_EQ(2, a1.use_count());
        EXPECT_TRUE(a1);
        EXPECT_EQ(1, *a1);
        EXPECT_EQ(2, a2.use_count());
        EXPECT_TRUE(a2);

        Shared_ptr<D> a3 = reinterpret_pointer_cast<D>(std::move(a1));
        EXPECT_EQ(0, a1.use_count());
        EXPECT_FALSE(a1);
        EXPECT_EQ(2, a3.use_count());
        EXPECT_TRUE(a3);
    }
}
