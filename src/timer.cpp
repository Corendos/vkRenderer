uint64_t get_time_ns() {
    struct timespec result = {};
    clock_gettime(CLOCK_MONOTONIC, &result);
    
    return result.tv_sec * 1000000000 + result.tv_nsec;
}