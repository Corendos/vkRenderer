#ifndef __MAT_H__
#define __MAT_H__

#include "math/vec.h"

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

Mat4f new_mat4f(f32 m00, f32 m01, f32 m02, f32 m03,
                f32 m10, f32 m11, f32 m12, f32 m13,
                f32 m20, f32 m21, f32 m22, f32 m23,
                f32 m30, f32 m31, f32 m32, f32 m33);

Mat4f identity_mat4f();
Mat4f perspective(f32 fov, f32 aspect, f32 near, f32 far);
Mat4f look_at(Vec3f eye, Vec3f origin, Vec3f up);
Mat4f look_from_yaw_and_pitch(Vec3f position, f32 yaw, f32 pitch, Vec3f up);

Mat4f scale_matrix(f32 scale_x, f32 scale_y, f32 scale_z);
Mat4f rotation_matrix(f32 angle_x, f32 angle_y, f32 angle_z);
Mat4f translation_matrix(f32 dx, f32 dy, f32 dz);

Mat4f inverse(Mat4f* a);
Mat4f transpose_inverse(Mat4f* a);

Vec4f mul(Mat4f* a, Vec4f* v);
Mat4f mul(Mat4f* a, Mat4f* b);

Vec4f operator*(Mat4f& m, Vec4f& v);
Mat4f operator*(Mat4f& m1, Mat4f& m2);

#endif //MAT_H
