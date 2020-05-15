#include "cg_string.h"
#include <string.h>
#include <stdarg.h>

inline ConstString make_const_string(char* str, u64 size) {
    ConstString result = {};
    result.str = str;
    result.size = size;
    
    return result;
}

inline ConstString make_const_string(char* str) {
    u64 size = strlen(str);
    return make_const_string(str, size);
}

inline ConstString make_const_string(String* string) {
    ConstString const_string = {};
    const_string.str = string->str;
    const_string.size = string->size;
    
    return const_string;
}

inline ConstString push_const_string(MemoryArena* storage, u64 size) {
    ConstString string = {};
    string.str = (char*)allocate(storage, size + 1);
    string.size = size;
    
    return string;
}

inline ConstString push_const_string(TemporaryMemory* temporary_storage, u64 size) {
    return push_const_string(temporary_storage->arena, size);
}

inline String make_string(char* str, u64 cap) {
    String string = {};
    string.str = str;
    string.size = 0;
    string.cap = cap;
    
    return string;
}

inline String push_string(MemoryArena* storage, u64 size) {
    String string = {};
    string.str = (char*)allocate(storage, size + 1);
    string.size = 0;
    string.cap = size;
    
    return string;
}

inline String push_string(TemporaryMemory* temporary_storage, u64 size) {
    return push_string(temporary_storage->arena, size);
}

inline ConstString push_string_copy(MemoryArena* storage, ConstString* src) {
    ConstString string = {};
    string.str = (char*)allocate(storage, src->size + 1);
    string.size = src->size;
    memcpy(string.str, src->str, src->size);
    return string;
}

inline ConstString push_string_copy(TemporaryMemory* temporary_storage, ConstString* src) {
    return push_string_copy(temporary_storage->arena, src);
}
