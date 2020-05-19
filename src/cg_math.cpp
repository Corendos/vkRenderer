#include "math.h"

#include <math.h>
#include <stdlib.h>

#include "math/vec.cpp"
#include "math/rect.cpp"
#include "math/mat.cpp"

inline f32 clamp(f32 value, f32 a, f32 b) {
    if (value > b) {
        return b;
    }
    if (value < a) {
        return a;
    }
    return value;
}

inline f32 randf() {
    return (f32)rand() / (f32)RAND_MAX;
}


inline char* to_string(Vec2f v, MemoryArena* temporary_storage, u32 indentation_level) {
    char* indent_space = (char*)allocate(temporary_storage, indentation_level + 1);
    for (u32 i = 0;i < indentation_level;i++) {
        indent_space[i] = ' ';
    }
    indent_space[indentation_level] = 0;
    char* str = (char*)allocate(temporary_storage, 100);
    
    sprintf(str,
            "Vec2f {\n"
            "%s    x: %06.6f\n"
            "%s    y: %06.6f\n"
            "%s}",
            indent_space, v.x,
            indent_space, v.y,
            indent_space);
    
    return str;
}

inline char* to_string(Vec3f v, MemoryArena* temporary_storage, u32 indentation_level) {
    char* indent_space = (char*)allocate(temporary_storage, indentation_level + 1);
    for (u32 i = 0;i < indentation_level;i++) {
        indent_space[i] = ' ';
    }
    indent_space[indentation_level] = 0;
    char* str = (char*)allocate(temporary_storage, 100);
    
    sprintf(str,
            "Vec2f {\n"
            "%s    x: %06.6f\n"
            "%s    y: %06.6f\n"
            "%s    z: %06.6f\n"
            "%s}",
            indent_space, v.x,
            indent_space, v.y,
            indent_space, v.z,
            indent_space);
    
    return str;
}

inline char* to_string(Vec4f v, MemoryArena* temporary_storage, u32 indentation_level) {
    char* indent_space = (char*)allocate(temporary_storage, indentation_level + 1);
    for (u32 i = 0;i < indentation_level;i++) {
        indent_space[i] = ' ';
    }
    indent_space[indentation_level] = 0;
    char* str = (char*)allocate(temporary_storage, 100);
    
    sprintf(str,
            "Vec2f {\n"
            "%s    x: %06.6f\n"
            "%s    y: %06.6f\n"
            "%s    z: %06.6f\n"
            "%s    w: %06.6f\n"
            "%s}",
            indent_space, v.x,
            indent_space, v.y,
            indent_space, v.z,
            indent_space, v.w,
            indent_space);
    
    return str;
}

inline char* to_string(Vec2u v, MemoryArena* temporary_storage, u32 indentation_level) {
    char* indent_space = (char*)allocate(temporary_storage, indentation_level + 1);
    for (u32 i = 0;i < indentation_level;i++) {
        indent_space[i] = ' ';
    }
    indent_space[indentation_level] = 0;
    char* str = (char*)allocate(temporary_storage, 100);
    
    sprintf(str,
            "Vec2f {\n"
            "%s    x: %u\n"
            "%s    y: %u\n"
            "%s}",
            indent_space, v.x,
            indent_space, v.y,
            indent_space);
    
    return str;
}

inline char* to_string(Vec3u v, MemoryArena* temporary_storage, u32 indentation_level) {
    char* indent_space = (char*)allocate(temporary_storage, indentation_level + 1);
    for (u32 i = 0;i < indentation_level;i++) {
        indent_space[i] = ' ';
    }
    indent_space[indentation_level] = 0;
    char* str = (char*)allocate(temporary_storage, 100);
    
    sprintf(str,
            "Vec2f {\n"
            "%s    x: %u\n"
            "%s    y: %u\n"
            "%s    z: %u\n"
            "%s}",
            indent_space, v.x,
            indent_space, v.y,
            indent_space, v.z,
            indent_space);
    
    return str;
}

inline char* to_string(Vec4u v, MemoryArena* temporary_storage, u32 indentation_level) {
    char* indent_space = (char*)allocate(temporary_storage, indentation_level + 1);
    for (u32 i = 0;i < indentation_level;i++) {
        indent_space[i] = ' ';
    }
    indent_space[indentation_level] = 0;
    char* str = (char*)allocate(temporary_storage, 100);
    
    sprintf(str,
            "Vec2f {\n"
            "%s    x: %u\n"
            "%s    y: %u\n"
            "%s    z: %u\n"
            "%s    w: %u\n"
            "%s}",
            indent_space, v.x,
            indent_space, v.y,
            indent_space, v.z,
            indent_space, v.w,
            indent_space);
    
    return str;
}

inline char* to_string(Vec2i v, MemoryArena* temporary_storage, u32 indentation_level) {
    char* indent_space = (char*)allocate(temporary_storage, indentation_level + 1);
    for (u32 i = 0;i < indentation_level;i++) {
        indent_space[i] = ' ';
    }
    indent_space[indentation_level] = 0;
    char* str = (char*)allocate(temporary_storage, 100);
    
    sprintf(str,
            "Vec2f {\n"
            "%s    x: %d\n"
            "%s    y: %d\n"
            "%s}",
            indent_space, v.x,
            indent_space, v.y,
            indent_space);
    
    return str;
}

inline char* to_string(Vec3i v, MemoryArena* temporary_storage, u32 indentation_level) {
    char* indent_space = (char*)allocate(temporary_storage, indentation_level + 1);
    for (u32 i = 0;i < indentation_level;i++) {
        indent_space[i] = ' ';
    }
    indent_space[indentation_level] = 0;
    char* str = (char*)allocate(temporary_storage, 100);
    
    sprintf(str,
            "Vec2f {\n"
            "%s    x: %d\n"
            "%s    y: %d\n"
            "%s    z: %d\n"
            "%s}",
            indent_space, v.x,
            indent_space, v.y,
            indent_space, v.z,
            indent_space);
    
    return str;
}

inline char* to_string(Vec4i v, MemoryArena* temporary_storage, u32 indentation_level) {
    char* indent_space = (char*)allocate(temporary_storage, indentation_level + 1);
    for (u32 i = 0;i < indentation_level;i++) {
        indent_space[i] = ' ';
    }
    indent_space[indentation_level] = 0;
    char* str = (char*)allocate(temporary_storage, 100);
    
    sprintf(str,
            "Vec2f {\n"
            "%s    x: %d\n"
            "%s    y: %d\n"
            "%s    z: %d\n"
            "%s    w: %d\n"
            "%s}",
            indent_space, v.x,
            indent_space, v.y,
            indent_space, v.z,
            indent_space, v.w,
            indent_space);
    
    return str;
}

inline f32 min(f32 a, f32 b) {
    return a < b ? a : b;
}

inline f32 max(f32 a, f32 b) {
    return a < b ? b : a;
}