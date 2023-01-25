#include <benchmark/benchmark.h>

#include <cstdint>
#include <vector>
#include <memory>

#include <memoc/allocators.h>

struct Test_data {
    std::vector<std::int64_t> allocation_sizes{};
    std::vector<std::int64_t> choosen_size_indices{};
};

template <std::int64_t Min_allocation_size, std::int64_t Max_allocation_size, std::int64_t Number_of_allocations>
Test_data test_data()
{
    static_assert(Min_allocation_size % 2 == 0);
    static_assert(Max_allocation_size % 2 == 0);
    static_assert(Number_of_allocations % 2 == 0);

    Test_data td;

    for (std::int64_t i = Min_allocation_size; i <= Max_allocation_size; i *= 2) {
        td.allocation_sizes.push_back(i);
    }

    for (std::int64_t i = 0; i < Number_of_allocations; ++i) {
        td.choosen_size_indices.push_back(i % td.allocation_sizes.size());
    }

    return td;
}

static void BM_default_allocator(benchmark::State& state)
{
    std::allocator<std::uint8_t> alloc{};
    auto td = test_data<16, 64, 64>();

    for (auto _ : state) {
        for (std::int64_t i : td.choosen_size_indices) {
            auto p = alloc.allocate(td.allocation_sizes[i]);
            alloc.deallocate(p, td.allocation_sizes[i]);
        }
    }
}
BENCHMARK(BM_default_allocator);

template <class Allocator>
void perform_allocations(Allocator* alloc, const Test_data& td) {
    for (std::int64_t i : td.choosen_size_indices) {
        auto b = alloc->allocate(td.allocation_sizes[i]);
        alloc->deallocate(b);
    }
}

static void BM_malloc_allocator(benchmark::State& state)
{
    memoc::Malloc_allocator alloc{};
    auto td = test_data<16, 64, 64>();

    for (auto _ : state) {
        perform_allocations(&alloc, td);
    }
}
BENCHMARK(BM_malloc_allocator);

static void BM_malloc_allocator_with_stats(benchmark::State& state)
{
    using namespace memoc;
    Stats_allocator<Malloc_allocator, 32> alloc{};
    auto td = test_data<16, 64, 64>();

    for (auto _ : state) {
        perform_allocations(&alloc, td);
    }
}
BENCHMARK(BM_malloc_allocator_with_stats);

static void BM_malloc_allocator_shared(benchmark::State& state)
{
    using namespace memoc;
    Shared_allocator<Malloc_allocator> alloc{};
    auto td = test_data<16, 64, 64>();

    for (auto _ : state) {
        perform_allocations(&alloc, td);
    }
}
BENCHMARK(BM_malloc_allocator_shared);

static void BM_stack_allocator(benchmark::State& state)
{
    memoc::Stack_allocator<64 * 64> alloc{};
    auto td = test_data<16, 64, 64>();

    for (auto _ : state) {
        perform_allocations(&alloc, td);
    }
}
BENCHMARK(BM_stack_allocator);

static void BM_free_list_allocator(benchmark::State& state)
{
    using namespace memoc;

    Free_list_allocator<Malloc_allocator, 16, 64, 64> alloc{};
    auto td = test_data<16, 64, 64>();

    for (auto _ : state) {
        perform_allocations(&alloc, td);
    }
}
BENCHMARK(BM_free_list_allocator);

static void BM_hybrid_allocator(benchmark::State& state)
{
    using namespace memoc;

    Fallback_allocator<
        Stack_allocator<16 * 16>,
        Free_list_allocator<Malloc_allocator, 16, 64, 16>> alloc{};
    auto td = test_data<16, 64, 64>();

    for (auto _ : state) {
        perform_allocations(&alloc, td);
    }
}
BENCHMARK(BM_hybrid_allocator);

template <class Allocator, typename T, std::int64_t Number_of_allocations>
void perform_vector_allocations() {
    std::vector<T, Allocator> v{};
    for (std::int64_t i = 0; i < Number_of_allocations; ++i) {
        v.push_back(static_cast<T>(i));
    }
}

static void BM_stl_default_allocator(benchmark::State& state)
{
    using namespace memoc;

    using Allocator = std::allocator<int>;

    for (auto _ : state) {
        perform_vector_allocations<Allocator, int, 1024>();
    }
}
BENCHMARK(BM_stl_default_allocator);

static void BM_stl_adapter_allocator(benchmark::State& state)
{
    using namespace memoc;

    using Allocator = Stl_adapter_allocator<int, Fallback_allocator<
        Stack_allocator<16 * 16>,
        Free_list_allocator<Malloc_allocator, 16, 64, 16>>>;

    for (auto _ : state) {
        perform_vector_allocations<Allocator, int, 1024>();
    }
}
BENCHMARK(BM_stl_adapter_allocator);

