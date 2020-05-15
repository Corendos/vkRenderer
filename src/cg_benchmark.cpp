inline void reset_benchmark(Benchmark* benchmark) {
    benchmark->count = 0;
}

inline void push_timestamp(Benchmark* benchmark) {
    benchmark->timestamps[benchmark->count++] = get_time_ns();
}

inline void print_benchmark(Benchmark* benchmark, const char** names, u32 name_count) {
    assert(names != 0 || name_count == 0);
    println("Benchmark:");
    for (u32 i = 0; i < benchmark->count;++i) {
        u64 diff = benchmark->timestamps[i] - benchmark->timestamps[0];
        u32 ns = diff % 1000;
        diff /= 1000;
        u32 us = diff % 1000;
        diff /= 1000;
        u32 ms = diff % 1000;
        if (i < name_count) {
            println("    %s: %3dms %3dus %3dns", names[i], ms, us, ns);
        } else {
            println("    %s: %3dms %3dus %3dns", names[i], ms, us, ns);
        }
    }
}