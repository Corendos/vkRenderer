#ifndef __BENCHMARK_H__
#define __BENCHMARK_H__

struct Benchmark {
    u64 timestamps[1000];
    u32 count;
};

void reset_benchmark(Benchmark* benchmark);
void push_timestamp(Benchmark* benchmark);
void print_benchmark(Benchmark* benchmark, const char** names, u32 name_count);

#endif