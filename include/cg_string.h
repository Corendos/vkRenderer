#ifndef __CG_STRING_H__
#define __CG_STRING_H__

#include "cg_types.h"
#include "cg_memory_arena.h"
#include "cg_temporary_memory.h"

#define make_literal_string(str) make_const_string((char*)(str), sizeof((str)) - 1)
#define string_format(string, format, ...) \
(string).size = snprintf((string).str, (string).cap, format, ##__VA_ARGS__)

struct ConstString {
    char* str;
    u64 size;
};

struct String {
    union {
        ConstString string;
        struct {
            char* str;
            u64 size;
        };
    };
    u64 cap;
};

ConstString make_const_string(char* str, u64 size);
ConstString make_const_string(char* str);
ConstString make_const_string(String* string);

ConstString push_const_string(MemoryArena* storage, u64 size);
ConstString push_const_string(TemporaryMemory* temporary_storage, u64 size);

String make_string(char* str, u64 cap);

String push_string(MemoryArena* storage, u64 size);
String push_string(TemporaryMemory* temporary_storage, u64 size);

ConstString push_string_copy(MemoryArena* storage, ConstString* src);
ConstString push_string_copy(TemporaryMemory* temporary_storage, ConstString* src);

i32 string_compare(ConstString a, ConstString b);
i32 string_compare(String a, String b);

#endif //CG_STRING_H
