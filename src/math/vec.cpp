#include "math/vec.h"

inline Vec2f new_vec2f(f32 x, f32 y) {
    Vec2f v = {};
    v.x = x;
    v.y = y;
    
    return v;
}

inline Vec3f new_vec3f(f32 x, f32 y, f32 z) {
    Vec3f v = {};
    v.x = x;
    v.y = y;
    v.z = z;
    
    return v;
}

inline Vec4f new_vec4f(f32 x, f32 y, f32 z, f32 w) {
    Vec4f v = {};
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;
    
    return v;
}

inline Vec2u new_vec2u(u32 x, u32 y) {
    Vec2u v = {};
    v.x = x;
    v.y = y;
    return v;
}

inline Vec3u new_vec3u(u32 x, u32 y, u32 z) {
    Vec3u v = {};
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

inline Vec4u new_vec4u(u32 x, u32 y, u32 z, u32 w) {
    Vec4u v = {};
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;
    return v;
}

inline Vec2i new_vec2i(i32 x, i32 y) {
    Vec2i v = {};
    v.x = x;
    v.y = y;
    return v;
}

inline Vec3i new_vec3i(i32 x, i32 y, i32 z) {
    Vec3i v = {};
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

inline Vec4i new_vec4i(i32 x, i32 y, i32 z, i32 w) {
    Vec4i v = {};
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;
    return v;
}

inline f32 dot(Vec2f* a, Vec2f* b) {
    return a->x * b->x + a->y * b->y;
}

inline f32 dot(Vec3f* a, Vec3f* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

inline f32 dot(Vec4f* a, Vec4f* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
}

inline f32 length(Vec2f* a) {
    return sqrt(dot(a, a));
}

inline f32 length(Vec3f* a) {
    return sqrt(dot(a, a));
}

inline f32 length(Vec4f* a) {
    return sqrt(dot(a, a));
}

inline Vec3f cross(Vec3f* a, Vec3f* b) {
    return new_vec3f(a->y * b->z - a->z * b->y,
                     a->z * b->x - a->x * b->z,
                     a->x * b->y - a->y * b->x);
}

inline Vec2f normalize(Vec2f* a) {
    f32 l = length(a);
    if (l == 0.0) return *a;
    Vec2f v = {};
    
    v.x = a->x / l;
    v.y = a->y / l;
    
    return v;
}

inline Vec3f normalize(Vec3f* a) {
    f32 l = length(a);
    if (l == 0.0) return *a;
    
    Vec3f v = {};
    
    v.x = a->x / l;
    v.y = a->y / l;
    v.z = a->z / l;
    
    return v;
}

inline Vec4f normalize(Vec4f* a) {
    f32 l = length(a);
    if (l == 0.0) return *a;
    
    Vec4f v = {};
    
    v.x = a->x / l;
    v.y = a->y / l;
    v.z = a->z / l;
    v.w = a->w / l;
    
    return v;
}

inline Vec2f operator+(Vec2f& a, Vec2f& b) {
    return new_vec2f(a.x + b.x, a.y + b.y);
}

inline Vec3f operator+(Vec3f& a, Vec3f& b) {
    return new_vec3f(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline Vec4f operator+(Vec4f& a, Vec4f& b) {
    return new_vec4f(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

inline Vec2f operator-(Vec2f& a, Vec2f& b) {
    return new_vec2f(a.x - b.x, a.y - b.y);
}

inline Vec3f operator-(Vec3f& a, Vec3f& b) {
    return new_vec3f(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline Vec4f operator-(Vec4f& a, Vec4f& b) {
    return new_vec4f(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

inline Vec2f operator-(Vec2f& a) {
    return new_vec2f(-a.x, -a.y);
}

inline Vec3f operator-(Vec3f& a) {
    return new_vec3f(-a.x, -a.y, -a.z);
}

inline Vec4f operator-(Vec4f& a) {
    return new_vec4f(-a.x, -a.y, -a.z, -a.w);
}
