#ifndef __CG_TEMPORARY_MEMORY_H__
#define __CG_TEMPORARY_MEMORY_H__

#include "cg_memory_arena.h"

struct TemporaryMemory {
    MemoryArena* arena;
    u64 saved_usage;
};

TemporaryMemory make_temporary_memory(MemoryArena* arena);
void destroy_temporary_memory(TemporaryMemory* memory);

void* allocate(TemporaryMemory* memory, u64 size);
void* zero_allocate(TemporaryMemory* memory, u64 size);

#endif //CG_TEMPORARY_MEMORY_H
