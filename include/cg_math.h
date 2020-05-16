#ifndef __MATH_H__
#define __MATH_H__

#define PI 3.1415926535897932384626433832795028841971693993751058209749445
#define PI_2 PI / 2
#define PI_3 PI / 3
#define PI_4 PI / 4
#define PI_6 PI / 6

#include "cg_memory_arena.h"

struct Vec2f {
    union {
        struct { f32 x; f32 y; };
        struct { f32 r; f32 g; };
        f32 v[2];
    };
};

struct Vec3f {
    union {
        struct { f32 x; f32 y; f32 z; };
        struct { f32 r; f32 g; f32 b; };
        f32 v[3];
    };
};

struct Vec4f {
    union {
        struct { f32 x; f32 y; f32 z; f32 w; };
        struct { f32 r; f32 g; f32 b; f32 a; };
        f32 v[4];
    };
};

struct Mat4f {
    union {
        struct {
            f32 m00; f32 m10; f32 m20; f32 m30;
            f32 m01; f32 m11; f32 m21; f32 m31;
            f32 m02; f32 m12; f32 m22; f32 m32;
            f32 m03; f32 m13; f32 m23; f32 m33;
        };
        f32 v[16];
    };
};

struct Rect2f {
    union {
        struct {
            f32 left;
            f32 top;
            f32 right;
            f32 bottom;
        };
        struct {
            Vec2f p1;
            Vec2f p2;
        };
    };
};

struct Vec2u {
    union {
        struct { u32 x; u32 y; };
        struct { u32 r; u32 g; };
        u32 v[2];
    };
};

struct Vec3u {
    union {
        struct { u32 x; u32 y; u32 z; };
        struct { u32 r; u32 g; u32 b; };
        u32 v[3];
    };
};

struct Vec4u {
    union {
        struct { u32 x; u32 y; u32 z; u32 w; };
        struct { u32 r; u32 g; u32 b; u32 a; };
        u32 v[4];
    };
};

struct Rect2u {
    union {
        struct {
            u32 left;
            u32 top;
            u32 right;
            u32 bottom;
        };
        struct {
            Vec2u p1;
            Vec2u p2;
        };
    };
};

struct Vec2i {
    union {
        struct { i32 x; i32 y; };
        struct { i32 r; i32 g; };
        i32 v[2];
    };
};

struct Vec3i {
    union {
        struct { i32 x; i32 y; i32 z; };
        struct { i32 r; i32 g; i32 b; };
        i32 v[3];
    };
};

struct Vec4i {
    union {
        struct { i32 x; i32 y; i32 z; i32 w; };
        struct { i32 r; i32 g; i32 b; i32 a; };
        i32 v[4];
    };
};


struct Rect2i {
    union {
        struct {
            i32 left;
            i32 top;
            i32 right;
            i32 bottom;
        };
        struct {
            Vec2i p1;
            Vec2i p2;
        };
    };
};

Vec2f new_vec2f(f32 x = 0.0f, f32 y = 0.0f);
Vec3f new_vec3f(f32 x = 0.0f, f32 y = 0.0f, f32 z = 0.0f);
Vec4f new_vec4f(f32 x = 0.0f, f32 y = 0.0f, f32 z = 0.0f, f32 w = 0.0f);

Mat4f new_mat4f(f32 m00, f32 m01, f32 m02, f32 m03,
                f32 m10, f32 m11, f32 m12, f32 m13,
                f32 m20, f32 m21, f32 m22, f32 m23,
                f32 m30, f32 m31, f32 m32, f32 m33);

Rect2f new_rect2f(f32 left, f32 top, f32 right, f32 bottom);
Rect2f new_rect2f(Vec2f p1, Vec2f p2);

Vec2u new_vec2u(u32 x = 0, u32 y = 0);
Vec3u new_vec3u(u32 x = 0, u32 y = 0, u32 z = 0);
Vec4u new_vec4u(u32 x = 0, u32 y = 0, u32 z = 0, u32 w = 0);

Rect2u new_rect2u(u32 left, u32 top, u32 right, u32 bottom);
Rect2u new_rect2u(Vec2u p1, Vec2u p2);

Vec2i new_vec2i(i32 x = 0, i32 y = 0);
Vec3i new_vec3i(i32 x = 0, i32 y = 0, i32 z = 0);
Vec4i new_vec4i(i32 x = 0, i32 y = 0, i32 z = 0, i32 w = 0);

Rect2i new_rect2i(i32 left, i32 top, i32 right, i32 bottom);
Rect2i new_rect2i(Vec2i p1, Vec2i p2);

Vec2f get_rect_dimensions(Rect2f* rectangle);

Mat4f identity_mat4f();
Mat4f perspective(f32 fov, f32 aspect, f32 near, f32 far);
Mat4f look_at(Vec3f eye, Vec3f origin, Vec3f up);
Mat4f look_from_yaw_and_pitch(Vec3f position, f32 yaw, f32 pitch, Vec3f up);

Mat4f scale_matrix(f32 scale_x, f32 scale_y, f32 scale_z);
Mat4f rotation_matrix(f32 angle_x, f32 angle_y, f32 angle_z);
Mat4f translation_matrix(f32 dx, f32 dy, f32 dz);

Mat4f mul(Mat4f* a, Mat4f* b);
Vec4f mul(Mat4f* a, Vec4f* v);

f32 dot(Vec2f *a, Vec2f *b);
f32 dot(Vec3f *a, Vec3f *b);
f32 dot(Vec4f *a, Vec4f *b);
Vec3f cross(Vec3f *a, Vec3f *b);

f32 length(Vec2f* a);
f32 length(Vec3f* a);
f32 length(Vec4f* a);

Vec2f normalize(Vec2f* a);
Vec3f normalize(Vec3f* a);
Vec4f normalize(Vec4f* a);

Vec2f operator+(Vec2f& a, Vec2f& b);
Vec3f operator+(Vec3f& a, Vec3f& b);
Vec4f operator+(Vec4f& a, Vec4f& b);

Vec2f operator-(Vec2f& a, Vec2f& b);
Vec3f operator-(Vec3f& a, Vec3f& b);
Vec4f operator-(Vec4f& a, Vec4f& b);

Vec2f operator-(Vec2f& a);
Vec3f operator-(Vec3f& a);
Vec4f operator-(Vec4f& a);

Vec4f operator*(Mat4f& m, Vec4f& v);
Mat4f operator*(Mat4f& m1, Mat4f& m2);

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
