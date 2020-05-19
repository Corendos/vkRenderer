#ifndef __VEC_H__
#define __VEC_H__

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


Vec2f new_vec2f(f32 x = 0.0f, f32 y = 0.0f);
Vec3f new_vec3f(f32 x = 0.0f, f32 y = 0.0f, f32 z = 0.0f);
Vec4f new_vec4f(f32 x = 0.0f, f32 y = 0.0f, f32 z = 0.0f, f32 w = 0.0f);

Vec2u new_vec2u(u32 x = 0, u32 y = 0);
Vec3u new_vec3u(u32 x = 0, u32 y = 0, u32 z = 0);
Vec4u new_vec4u(u32 x = 0, u32 y = 0, u32 z = 0, u32 w = 0);

Vec2i new_vec2i(i32 x = 0, i32 y = 0);
Vec3i new_vec3i(i32 x = 0, i32 y = 0, i32 z = 0);
Vec4i new_vec4i(i32 x = 0, i32 y = 0, i32 z = 0, i32 w = 0);

f32 dot(Vec2f *a, Vec2f *b);
f32 dot(Vec3f *a, Vec3f *b);
f32 dot(Vec4f *a, Vec4f *b);

f32 length(Vec2f* a);
f32 length(Vec3f* a);
f32 length(Vec4f* a);

Vec3f cross(Vec3f *a, Vec3f *b);

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

#endif //VEC_H
