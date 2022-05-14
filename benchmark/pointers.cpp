#include <benchmark/benchmark.h>

#include <memory>
#include <array>

#include <math/core/pointers.h>

static void BM_std_shared_ptr(benchmark::State& state)
{
    for (auto _ : state) {
        std::shared_ptr<int> sp1 = std::make_shared<int>(1998);
        benchmark::DoNotOptimize(*sp1);
        benchmark::DoNotOptimize(sp1.use_count());
        {
            std::shared_ptr<int> sp2{ sp1 };
            benchmark::DoNotOptimize(sp2.use_count());
        }
        benchmark::DoNotOptimize(sp1.use_count());
        std::shared_ptr<int> sp3 = sp1;
        benchmark::DoNotOptimize(sp1.use_count());
        sp3.reset();
        benchmark::DoNotOptimize(sp1.use_count());
        sp1 = std::make_shared<int>(2011);
    }
}
BENCHMARK(BM_std_shared_ptr);

static void BM_LW_shared_ptr(benchmark::State& state)
{
    using namespace math::core::pointers;

    for (auto _ : state) {
        Shared_ptr<int> sp1 = Shared_ptr<int>::make_shared(1998);
        benchmark::DoNotOptimize(*sp1);
        benchmark::DoNotOptimize(sp1.use_count());
        {
            Shared_ptr<int> sp2{ sp1 };
            benchmark::DoNotOptimize(sp2.use_count());
        }
        benchmark::DoNotOptimize(sp1.use_count());
        Shared_ptr<int> sp3 = sp1;
        benchmark::DoNotOptimize(sp1.use_count());
        sp3.reset();
        benchmark::DoNotOptimize(sp1.use_count());
        sp1 = Shared_ptr<int>::make_shared(2011);
    }
}
BENCHMARK(BM_LW_shared_ptr);
