#include "cg_random.h"

#include <stdlib.h>

inline void init_random() {
    init_random(get_time_ns());
}

inline void init_random(u64 seed) {
    // NOTE: this is an unchecked cast. It may be problematic.
    srand((u32)seed);
}

inline u32 randlim(u32 limit) {
    /* This is extracted from :
---https://stackoverflow.com/questions/2999075/generate-a-random-number-within-range/2999130#2999130
This may include non determinism, but the loop is actually ran O(1) times.
*/
    u32 divisor = RAND_MAX / (limit + 1);
    u32 retval = 0;
    
    do {
        retval = rand() / divisor;
    } while(retval > limit);
    
    return retval;
}

inline u32 randrange(u32 a, u32 b) {
    return randlim(b - a) + a;
}

inline ConstString random_temp_word(MemoryArena* memory, u64 len) {
    ConstString word = push_const_string(memory, len);
    
    for (u32 i = 0;i < len;++i) {
        char* c = word.str + i;
        *c = (char)randrange((u32)'a', (u32)'z');
    }
    
    word.str[len] = '\0';
    return word;
}