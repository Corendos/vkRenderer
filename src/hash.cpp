#include "hash.hpp"

u64 hash(const char* str) {
    u64 hash = 0;
    int c;
    
    while (c = *str++) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    
    return hash;
}
