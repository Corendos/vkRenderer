#ifndef __MATH_H__
#define __MATH_H__

#include <float.h>

#define PI 3.1415926535897932384626433832795028841971693993751058209749445
#define PI_2 PI / 2
#define PI_3 PI / 3
#define PI_4 PI / 4
#define PI_6 PI / 6


#include "cg_memory_arena.h"

#include "math/vec.h"
#include "math/rect.h"
#include "math/mat.h"

f32 clamp(f32 value, f32 a, f32 b);
f32 randf();

char* to_string(Vec2f v, MemoryArena* temporary_storage, u32 indentation_level = 0);
char* to_string(Vec3f v, MemoryArena* temporary_storage, u32 indentation_level = 0);
char* to_string(Vec4f v, MemoryArena* temporary_storage, u32 indentation_level = 0);

char* to_string(Vec2u v, MemoryArena* temporary_storage, u32 indentation_level = 0);
char* to_string(Vec3u v, MemoryArena* temporary_storage, u32 indentation_level = 0);
char* to_string(Vec4u v, MemoryArena* temporary_storage, u32 indentation_level = 0);

char* to_string(Vec2i v, MemoryArena* temporary_storage, u32 indentation_level = 0);
char* to_string(Vec3i v, MemoryArena* temporary_storage, u32 indentation_level = 0);
char* to_string(Vec4i v, MemoryArena* temporary_storage, u32 indentation_level = 0);

f32 min(f32 a, f32 b);
f32 max(f32 a, f32 b);

#endif
