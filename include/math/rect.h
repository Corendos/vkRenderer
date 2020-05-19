#ifndef __RECT_H__
#define __RECT_H__

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

Rect2f new_rect2f(f32 left, f32 top, f32 right, f32 bottom);
Rect2f new_rect2f(Vec2f p1, Vec2f p2);
Rect2f new_rect2f_dim(f32 left, f32 top, f32 width, f32 height);
Rect2f new_rect2f(Vec2f origin, Vec2f dim);

Rect2u new_rect2u(u32 left, u32 top, u32 right, u32 bottom);
Rect2u new_rect2u(Vec2u p1, Vec2u p2);
Rect2u new_rect2u_dim(u32 left, u32 top, u32 width, u32 height);
Rect2u new_rect2u(Vec2u origin, Vec2u dim);

Rect2i new_rect2i(i32 left, i32 top, i32 right, i32 bottom);
Rect2i new_rect2i(Vec2i p1, Vec2i p2);
Rect2i new_rect2i_dim(i32 left, i32 top, i32 width, i32 height);
Rect2i new_rect2i(Vec2i origin, Vec2i dim);

Vec2f get_rect_dimensions(Rect2f* rectangle);

#endif //RECT_H
