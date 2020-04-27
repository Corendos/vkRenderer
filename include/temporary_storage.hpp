#ifndef __TEMPORARY_STORAGE_HPP__
#define __TEMPORARY_STORAGE_HPP__

#include "macros.hpp"

#define TEMPORARY_STORAGE_SIZE mega(1)

struct TemporaryStorage {
    void* data;
    uint32_t size;
    uint32_t usage;
    uint32_t outside_usage;
    uint32_t max_usage;
};

bool init_temporary_storage(TemporaryStorage* temporary_storage);
void destroy_temporary_storage(TemporaryStorage* temporary_storage, bool verbose = true);
void* allocate(TemporaryStorage* temporary_storage, uint32_t size);
void* zero_allocate(TemporaryStorage* temporary_storage, uint32_t size);
void reset(TemporaryStorage* temporary_storage);
char* to_string(TemporaryStorage to_print, TemporaryStorage* temporary_storage, uint32_t indentation_level = 0);

#endif