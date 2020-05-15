#include "cg_memory_arena.h"

inline bool init_memory_arena(MemoryArena* arena, u64 size) {
    arena->data = calloc(size, 1);
    arena->size = size;
    return (arena->data != 0);
}

inline void destroy_memory_arena(MemoryArena* arena, bool verbose) {
    if (verbose) {
        println("Destroying memory arena");
    }
    free(arena->data);
}

inline void* allocate(MemoryArena* arena, u64 size) {
    void* data = {};
    if (arena->usage + size < arena->size) {
        data = (u8*)arena->data + arena->usage;
        arena->usage += size;
    } else {
        data = malloc(size);
        println("Memory Arena overflow");
        arena->outside_usage += size;
    }
    
    if (arena->outside_usage + arena->usage > arena->max_usage) {
        arena->max_usage = arena->outside_usage + arena->usage;
    }
    
    return data;
}

inline void* zero_allocate(MemoryArena* arena, u64 size) {
    void* data = allocate(arena, size);
    for (u8* d = (u8*)data;d < (u8*)data + size;++d) {
        *d = 0;
    }
    
    return data;
}

inline void reset_arena(MemoryArena* arena) {
    arena->usage = 0;
    arena->outside_usage = 0;
}

inline char* to_string(MemoryArena to_print, MemoryArena* arena, u64 indentation_level){
    char* indent_space = (char*)allocate(arena, indentation_level + 1);
    for (u32 i = 0;i < indentation_level;i++) {
        indent_space[i] = ' ';
    }
    indent_space[indentation_level] = 0;
    char* str = (char*)allocate(arena, 10000);
    
    sprintf(str,
            "MemoryArena {\n"
            "%s    data: %p\n"
            "%s    size: %ld\n"
            "%s    usage: %ld\n"
            "%s    outside_usage: %ld\n"
            "%s    max_usage: %ld\n"
            "%s}",
            indent_space, to_print.data,
            indent_space, to_print.size,
            indent_space, to_print.usage,
            indent_space, to_print.outside_usage,
            indent_space, to_print.max_usage,
            indent_space);
    
    return str;
}