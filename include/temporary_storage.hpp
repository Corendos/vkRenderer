#ifndef __TEMPORARY_STORAGE_HPP__
#define __TEMPORARY_STORAGE_HPP__

#include "macros.hpp"

#define TEMPORARY_STORAGE_SIZE MB(1)

struct TemporaryStorage {
    void* data;
    u64 size;
    u64 usage;
    u64 outside_usage;
    u64 max_usage;
};

bool init_temporary_storage(TemporaryStorage* temporary_storage, u64 size = TEMPORARY_STORAGE_SIZE);
void destroy_temporary_storage(TemporaryStorage* temporary_storage, bool verbose = true);
void* allocate(TemporaryStorage* temporary_storage, u64 size);
void* zero_allocate(TemporaryStorage* temporary_storage, u64 size);
void reset(TemporaryStorage* temporary_storage);
char* to_string(TemporaryStorage to_print, TemporaryStorage* temporary_storage, u32 indentation_level = 0);

#endif