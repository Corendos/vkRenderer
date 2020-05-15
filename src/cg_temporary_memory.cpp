#include "cg_temporary_memory.h"

inline TemporaryMemory make_temporary_memory(MemoryArena* arena) {
    TemporaryMemory memory = {};
    memory.arena = arena;
    memory.saved_usage = arena->usage;
    
    return memory;
}

inline void destroy_temporary_memory(TemporaryMemory* memory) {
    memory->arena->usage = memory->saved_usage;
    
    memory->arena = 0;
    memory->saved_usage = 0;
}

inline void* allocate(TemporaryMemory* memory, u64 size) {
    return allocate(memory->arena, size);
}

inline void* zero_allocate(TemporaryMemory* memory, u64 size) {
    return zero_allocate(memory->arena, size);
}
