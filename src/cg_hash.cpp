#include "cg_hash.h"

inline u64 hash(const char* str) {
    u64 hash = 0;
    int c;
    
    while (c = *str++) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    
    return hash;
}

inline u64 hash(ConstString str) {
    return hash(str.str);
}

inline u64 hash(const u8* data, u32 count) {
    u64 hash = 0;
    int c;
    
    for (u32 i = 0;i < count;++i) {
        c = *data++;
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    
    return hash;
}