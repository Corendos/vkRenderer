#ifndef __MATH_HPP__
#define __MATH_HPP__

#define PI 3.1415926535897932384626433832795028841971693993751058209749445
#define PI_2 PI / 2
#define PI_3 PI / 3
#define PI_4 PI / 4
#define PI_6 PI / 6

struct Vec2f {
    union {
	struct { float x; float y; };
	struct { float r; float g; };
	float v[2];
    };
};

struct Vec3f {
    union {
	struct { float x; float y; float z; };
	struct { float r; float g; float b; };
	float v[3];
    };
};

struct Vec4f {
    union {
	struct { float x; float y; float z; float w; };
	struct { float r; float g; float b; float a; };
	float v[4];
    };
};

struct Mat4f {
    union {
	struct {
	    float m00; float m10; float m20; float m30;
	    float m01; float m11; float m21; float m31;
	    float m02; float m12; float m22; float m32;
	    float m03; float m13; float m23; float m33;
	};
	float v[16];
    };
};

Vec2f new_vec2f(float x = 0.0f, float y = 0.0f);
Vec3f new_vec3f(float x = 0.0f, float y = 0.0f, float z = 0.0f);
Vec4f new_vec4f(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 0.0f);

Mat4f new_mat4f(float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33);

Mat4f identity_mat4f();
Mat4f perspective(float fov, float aspect, float near, float far);
Mat4f look_at(Vec3f eye, Vec3f origin, Vec3f up);

Mat4f scale_matrix(float scale_x, float scale_y, float scale_z);
Mat4f rotation_matrix(float angle_x, float angle_y, float angle_z);
Mat4f translation_matrix(float dx, float dy, float dz);

Mat4f mul(Mat4f* a, Mat4f* b);
Vec4f mul(Mat4f* a, Vec4f* v);

float dot(Vec2f *a, Vec2f *b);
float dot(Vec3f *a, Vec3f *b);
float dot(Vec4f *a, Vec4f *b);
Vec3f cross(Vec3f *a, Vec3f *b);

float length(Vec2f* a);
float length(Vec3f* a);
float length(Vec4f* a);

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


#endif
