#include <benchmark/benchmark.h>

#include <memory>
#include <array>

#include <math/core/pointers.h>

static void BM_std_shared_ptr(benchmark::State& state)
{
    std::array<std::shared_ptr<int>, 256> ptrs;
    std::shared_ptr<int> p = std::make_shared<int>(0);

    for (auto _ : state) {
        for (auto& ptr : ptrs) {
            ptr = p;
        }
        //for (auto& ptr : ptrs) {
        //    ptr.reset();
        //}
    }
}
BENCHMARK(BM_std_shared_ptr);

static void BM_LW_shared_ptr(benchmark::State& state)
{
    using namespace math::core::pointers;

    std::array<Shared_ptr<int>, 256> ptrs;
    Shared_ptr<int> p = Shared_ptr<int>::make_shared(0);

    for (auto _ : state) {
        for (auto& ptr : ptrs) {
            ptr = p;
        }
        //for (auto& ptr : ptrs) {
        //    ptr.reset();
        //}
    }
}
BENCHMARK(BM_LW_shared_ptr);
