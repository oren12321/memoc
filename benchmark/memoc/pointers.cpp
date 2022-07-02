#include <benchmark/benchmark.h>

#include <memory>
#include <array>

#include <memoc/pointers.h>

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
    using namespace memoc::pointers;

    for (auto _ : state) {
        Shared_ptr<int> sp1 = make_shared<int>(1998);
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
        sp1 = make_shared<int>(2011);
    }
}
BENCHMARK(BM_LW_shared_ptr);
