#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>

#include <math/core/allocators.h>

struct Test_data {
    std::vector<std::size_t> allocation_sizes{};
    std::vector<std::size_t> choosen_size_indices{};
};

template <std::size_t MinAllocationSize, std::size_t MaxAllocationSize, std::size_t NumberOfAllocations>
Test_data test_data()
{
    static_assert(MinAllocationSize % 2 == 0);
    static_assert(MaxAllocationSize % 2 == 0);
    static_assert(NumberOfAllocations % 2 == 0);

    Test_data td;

    for (std::size_t i = MinAllocationSize; i <= MaxAllocationSize; i *= 2) {
        td.allocation_sizes.push_back(i);
    }

    for (std::size_t i = 0; i < NumberOfAllocations; ++i) {
        td.choosen_size_indices.push_back(i % td.allocation_sizes.size());
    }

    return td;
}

static void BM_default_allocator(benchmark::State& state)
{
    std::allocator<std::uint8_t> alloc{};
    auto td = test_data<16, 64, 64>();

    for (auto _ : state) {
        for (std::size_t i : td.choosen_size_indices) {
            auto p = alloc.allocate(td.allocation_sizes[i]);
            alloc.deallocate(p, td.allocation_sizes[i]);
        }
    }
}
BENCHMARK(BM_default_allocator);

template <class Allocator>
void performAllocations(Allocator* alloc, const Test_data& td) {
    for (std::size_t i : td.choosen_size_indices) {
        auto b = alloc->allocate(td.allocation_sizes[i]);
        alloc->deallocate(&b);
    }
}

static void BM_malloc_allocator(benchmark::State& state)
{
    math::core::allocators::Malloc_allocator alloc{};
    auto td = test_data<16, 64, 64>();

    for (auto _ : state)
    {
        performAllocations(&alloc, td);
    }
}
BENCHMARK(BM_malloc_allocator);

static void BM_stack_allocator(benchmark::State& state)
{
    math::core::allocators::Stack_allocator<64 * 64> alloc{};
    auto td = test_data<16, 64, 64>();

    for (auto _ : state)
    {
        performAllocations(&alloc, td);
    }
}
BENCHMARK(BM_stack_allocator);

static void BM_free_list_allocator(benchmark::State& state)
{
    using namespace math::core::allocators;

    Free_list_allocator<Malloc_allocator, 16, 64, 64> alloc{};
    auto td = test_data<16, 64, 64>();

    for (auto _ : state)
    {
        performAllocations(&alloc, td);
    }
}
BENCHMARK(BM_free_list_allocator);

static void BM_hybrid_allocator(benchmark::State& state)
{
    using namespace math::core::allocators;

    Fallback_allocator<
        Stack_allocator<16 * 16>,
        Free_list_allocator<Malloc_allocator, 16, 64, 16>> alloc{};
    auto td = test_data<16, 64, 64>();

    for (auto _ : state)
    {
        performAllocations(&alloc, td);
    }
}
BENCHMARK(BM_hybrid_allocator);

static void BM_stl_default_allocator(benchmark::State& state)
{
    using namespace math::core::allocators;

    using Allocator = std::allocator<int>;
    std::vector<int, Allocator> v;

    for (auto _ : state)
    {
        for (std::size_t i = 0; i < 1024; ++i) {
            v.push_back(static_cast<int>(i));
        }
    }
}
BENCHMARK(BM_stl_default_allocator);

static void BM_stl_adapter_allocator(benchmark::State& state)
{
    using namespace math::core::allocators;

    using Allocator = Stl_adapter_allocator<int, Fallback_allocator<
        Stack_allocator<16 * 16>,
        Free_list_allocator<Malloc_allocator, 16, 64, 16>>>;
    std::vector<int, Allocator> v;

    for (auto _ : state)
    {
        for (std::size_t i = 0; i < 1024; ++i) {
            v.push_back(static_cast<int>(i));
        }
    }
}
BENCHMARK(BM_stl_adapter_allocator);

