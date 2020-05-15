#include "cg_timer.h"

#include <time.h>

inline u64 get_time_ns() {
    struct timespec result = {};
    clock_gettime(CLOCK_MONOTONIC, &result);
    
    return result.tv_sec * 1000000000 + result.tv_nsec;
}