#include "math.hpp"

#include <iostream>
#include <cmath>
#include <cstdlib>

Vec2f new_vec2f(float x, float y) {
    Vec2f v = {};
    v.x = x;
    v.y = y;

    return v;
}

Vec3f new_vec3f(float x, float y, float z) {
    Vec3f v = {};
    v.x = x;
    v.y = y;
    v.z = z;

    return v;
}

Vec4f new_vec4f(float x, float y, float z, float w) {
    Vec4f v = {};
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;

    return v;
}

Mat4f new_mat4f(float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33) {
    Mat4f m = {};

    m.m00 = m00;
    m.m01 = m01;
    m.m02 = m02;
    m.m03 = m03;

    m.m10 = m10;
    m.m11 = m11;
    m.m12 = m12;
    m.m13 = m13;

    m.m20 = m20;
    m.m21 = m21;
    m.m22 = m22;
    m.m23 = m23;

    m.m30 = m30;
    m.m31 = m31;
    m.m32 = m32;
    m.m33 = m33;

    return m;
}

Mat4f perspective(float fov, float aspect, float near, float far) {
    Mat4f m = {};

    float s = 1.0 / tan(fov / 2.0 * PI / 180.0f);

    m.m00 = s / aspect;
    m.m11 = -s;
    m.m22 = far / (near - far);
    m.m32 = -1.0;
    m.m23 = - far * near / (far - near);

    return m;
}

Mat4f identity_mat4f() {
    Mat4f m = {};

    m.m00 = 1.0f;
    m.m11 = 1.0f;
    m.m22 = 1.0f;
    m.m33 = 1.0f;

    return m;
}

Mat4f look_at(Vec3f eye, Vec3f origin, Vec3f up) {
    Mat4f m = {};

    Vec3f origin_to_eye = eye - origin;
    Vec3f forward = normalize(&origin_to_eye);
    Vec3f n_up = normalize(&up);
    Vec3f right = cross(&n_up, &forward);
    Vec3f _up = cross(&forward, &right);

    return new_mat4f(right.x,   right.y,   right.z,   -dot(&right, &eye),
		     _up.x,     _up.y,     _up.z,     -dot(&_up, &eye),
		     forward.x, forward.y, forward.z, -dot(&forward, &eye),
		     0,         0,         0,         1);
}

Mat4f look_from_yaw_and_pitch(Vec3f position, float yaw, float pitch, Vec3f up) {
    Mat4f m = {};
    Vec3f forward = new_vec3f(sin(yaw) * cos(pitch), sin(pitch), cos(yaw) * cos(pitch));
    Vec3f right = new_vec3f(cos(yaw), 0.0f, -sin(yaw));
    Vec3f _up = cross(&forward, &right);

    return new_mat4f(right.x,   right.y,   right.z,   -dot(&right, &position),
		     _up.x,     _up.y,     _up.z,     -dot(&_up, &position),
		     forward.x, forward.y, forward.z, -dot(&forward, &position),
		     0,         0,         0,         1);
}

Mat4f scale_matrix(float scale_x, float scale_y, float scale_z) {
    Mat4f m = {};

    m.m00 = scale_x;
    m.m11 = scale_y;
    m.m22 = scale_z;
    m.m33 = 1.0f;

    return m;
}

Mat4f rotation_matrix(float angle_x, float angle_y, float angle_z) {
    float cx = cos(angle_x);
    float sx = sin(angle_x);
    float cy = cos(angle_y);
    float sy = sin(angle_y);
    float cz = cos(angle_z);
    float sz = sin(angle_z);

    Mat4f m = {};
    m.m00 = cz * cy;
    m.m01 = cz * sy * sx - sz * cx;
    m.m02 = cz * sy * cx + sz * sx;

    m.m10 = sz * cy;
    m.m11 = sz * sy * sx + cz * cx;
    m.m12 = sz * sy * cx - cz * sx;

    m.m20 = -sy;
    m.m21 = cy * sx;
    m.m22 = cy * cx;

    m.m33 = 1.0f;

    return m;
}

Mat4f translation_matrix(float dx, float dy, float dz) {
    Mat4f m = {};

    m.m00 = 1.0f;
    m.m11 = 1.0f;
    m.m22 = 1.0f;
    m.m33 = 1.0f;

    m.m03 = dx;
    m.m13 = dy;
    m.m23 = dz;

    return m;
}

Mat4f mul(Mat4f* a, Mat4f* b) {
    Mat4f result = {};

    result.m00 = a->m00 * b->m00 + a->m01 * b->m10 + a->m02 * b->m20 + a->m03 * b->m30;
    result.m01 = a->m00 * b->m01 + a->m01 * b->m11 + a->m02 * b->m21 + a->m03 * b->m31;
    result.m02 = a->m00 * b->m02 + a->m01 * b->m12 + a->m02 * b->m22 + a->m03 * b->m32;
    result.m03 = a->m00 * b->m03 + a->m01 * b->m13 + a->m02 * b->m23 + a->m03 * b->m33;

    result.m10 = a->m10 * b->m00 + a->m11 * b->m10 + a->m12 * b->m20 + a->m13 * b->m30;
    result.m11 = a->m10 * b->m01 + a->m11 * b->m11 + a->m12 * b->m21 + a->m13 * b->m31;
    result.m12 = a->m10 * b->m02 + a->m11 * b->m12 + a->m12 * b->m22 + a->m13 * b->m32;
    result.m13 = a->m10 * b->m03 + a->m11 * b->m13 + a->m12 * b->m23 + a->m13 * b->m33;

    result.m20 = a->m20 * b->m00 + a->m21 * b->m10 + a->m22 * b->m20 + a->m23 * b->m30;
    result.m21 = a->m20 * b->m01 + a->m21 * b->m11 + a->m22 * b->m21 + a->m23 * b->m31;
    result.m22 = a->m20 * b->m02 + a->m21 * b->m12 + a->m22 * b->m22 + a->m23 * b->m32;
    result.m23 = a->m20 * b->m03 + a->m21 * b->m13 + a->m22 * b->m23 + a->m23 * b->m33;

    result.m30 = a->m30 * b->m00 + a->m31 * b->m10 + a->m32 * b->m20 + a->m33 * b->m30;
    result.m31 = a->m30 * b->m01 + a->m31 * b->m11 + a->m32 * b->m21 + a->m33 * b->m31;
    result.m32 = a->m30 * b->m02 + a->m31 * b->m12 + a->m32 * b->m22 + a->m33 * b->m32;
    result.m33 = a->m30 * b->m03 + a->m31 * b->m13 + a->m32 * b->m23 + a->m33 * b->m33;

    return result;
}

Vec4f mul(Mat4f* a, Vec4f* v) {
    Vec4f result = {};

    result.x = a->m00 * v->x + a->m01 * v->y + a->m02 * v->z + a->m03 * v->w;
    result.y = a->m10 * v->x + a->m11 * v->y + a->m12 * v->z + a->m13 * v->w;
    result.z = a->m20 * v->x + a->m21 * v->y + a->m22 * v->z + a->m23 * v->w;
    result.w = a->m30 * v->x + a->m31 * v->y + a->m32 * v->z + a->m33 * v->w;

    return result;
}

float dot(Vec2f* a, Vec2f* b) {
    return a->x * b->x + a->y * b->y;
}

float dot(Vec3f* a, Vec3f* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

float dot(Vec4f* a, Vec4f* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
}

Vec3f cross(Vec3f* a, Vec3f* b) {
    return new_vec3f(a->y * b->z - a->z * b->y,
		     a->z * b->x - a->x * b->z,
		     a->x * b->y - a->y * b->x);
}

float length(Vec2f* a) {
    return sqrt(dot(a, a));
}

float length(Vec3f* a) {
    return sqrt(dot(a, a));
}

float length(Vec4f* a) {
    return sqrt(dot(a, a));
}

Vec2f normalize(Vec2f* a) {
    float l = length(a);
    if (l == 0.0) return *a;
    Vec2f v = {};

    v.x = a->x / l;
    v.y = a->y / l;

    return v;
}

Vec3f normalize(Vec3f* a) {
    float l = length(a);
    if (l == 0.0) return *a;

    Vec3f v = {};

    v.x = a->x / l;
    v.y = a->y / l;
    v.z = a->z / l;

    return v;
}

Vec4f normalize(Vec4f* a) {
    float l = length(a);
    if (l == 0.0) return *a;

    Vec4f v = {};

    v.x = a->x / l;
    v.y = a->y / l;
    v.z = a->z / l;
    v.w = a->w / l;

    return v;
}



Vec2f operator+(Vec2f& a, Vec2f& b) {
    return new_vec2f(a.x + b.x, a.y + b.y);
}

Vec3f operator+(Vec3f& a, Vec3f& b) {
    return new_vec3f(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vec4f operator+(Vec4f& a, Vec4f& b) {
    return new_vec4f(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

Vec2f operator-(Vec2f& a, Vec2f& b) {
    return new_vec2f(a.x - b.x, a.y - b.y);
}

Vec3f operator-(Vec3f& a, Vec3f& b) {
    return new_vec3f(a.x - b.x, a.y - b.y, a.z - b.z);
}

Vec4f operator-(Vec4f& a, Vec4f& b) {
    return new_vec4f(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

Vec2f operator-(Vec2f& a) {
    return new_vec2f(-a.x, -a.y);
}

Vec3f operator-(Vec3f& a) {
    return new_vec3f(-a.x, -a.y, -a.z);
}

Vec4f operator-(Vec4f& a) {
    return new_vec4f(-a.x, -a.y, -a.z, -a.w);
}

Vec4f operator*(Mat4f& m, Vec4f& v) {
    return mul(&m, &v);
}

Mat4f operator*(Mat4f& m1, Mat4f& m2) {
    return mul(&m1, &m2);
}

float clamp(float value, float a, float b) {
    if (value > b) {
	return b;
    }
    if (value < a) {
	return a;
    }
    return value;
}

float randf() {
    return (float)rand() / (float)RAND_MAX;
}
