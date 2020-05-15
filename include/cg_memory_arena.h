#ifndef __CG_MEMORY_ARENA_H__
#define __CG_MEMORY_ARENA_H__

#include "cg_macros.h"
#define TEMPORARY_STORAGE_SIZE MB(1)

struct MemoryArena {
    void* data;
    u64 size;
    u64 usage;
    u64 outside_usage;
    u64 max_usage;
};

bool init_memory_arena(MemoryArena* arena, u64 size = TEMPORARY_STORAGE_SIZE);
void destroy_memory_arena(MemoryArena* arena, bool verbose = true);
void* allocate(MemoryArena* arena, u64 size);
void* zero_allocate(MemoryArena* arena, u64 size);
void reset_arena(MemoryArena* arena);
char* to_string(MemoryArena to_print, MemoryArena* arena, u32 indentation_level = 0);


#endif //CG_MEMORY_ARENA_H
