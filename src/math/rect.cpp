#include "math/rect.h"

inline Rect2f new_rect2f(f32 left, f32 top, f32 right, f32 bottom) {
    Rect2f rectangle = {};
    
    rectangle.left   = left;
    rectangle.top    = top;
    rectangle.right  = right;
    rectangle.bottom = bottom;
    
    return rectangle;
}

inline Rect2f new_rect2f(Vec2f p1, Vec2f p2) {
    Rect2f rectangle = {};
    
    rectangle.p1 = p1;
    rectangle.p2 = p2;
    
    return rectangle;
}

inline Rect2f new_rect2f_dim(f32 left, f32 top, f32 width, f32 height) {
    Rect2f rectangle = {};
    
    rectangle.left   = left;
    rectangle.top    = top;
    rectangle.right  = left + width - 1;
    rectangle.bottom = top + height - 1;
    
    return rectangle;
}

inline Rect2f new_rect2f_dim(Vec2f p1, Vec2f p2) {
    Rect2f rectangle = {};
    
    rectangle.p1 = p1;
    rectangle.p2 = { p1.x + p2.x - 1, p1.y + p2.y - 1 };
    
    return rectangle;
}

inline Rect2u new_rect2u(u32 left, u32 top, u32 right, u32 bottom) {
    Rect2u rectangle = {};
    
    rectangle.left   = left;
    rectangle.top    = top;
    rectangle.right  = right;
    rectangle.bottom = bottom;
    
    return rectangle;
}

inline Rect2u new_rect2u(Vec2u p1, Vec2u p2) {
    Rect2u rectangle = {};
    
    rectangle.p1 = p1;
    rectangle.p2 = p2;
    
    return rectangle;
}

inline Rect2u new_rect2u_dim(u32 left, u32 top, u32 width, u32 height) {
    Rect2u rectangle = {};
    
    rectangle.left   = left;
    rectangle.top    = top;
    rectangle.right  = left + width - 1;
    rectangle.bottom = top + height - 1;
    
    return rectangle;
}

inline Rect2u new_rect2u_dim(Vec2u p1, Vec2u p2) {
    Rect2u rectangle = {};
    
    rectangle.p1 = p1;
    rectangle.p2 = { p1.x + p2.x - 1, p1.y + p2.y - 1 };
    
    return rectangle;
}

inline Rect2i new_rect2i(i32 left, i32 top, i32 right, i32 bottom) {
    Rect2i rectangle = {};
    
    rectangle.left   = left;
    rectangle.top    = top;
    rectangle.right  = right;
    rectangle.bottom = bottom;
    
    return rectangle;
}

inline Rect2i new_rect2i(Vec2i p1, Vec2i p2) {
    Rect2i rectangle = {};
    
    rectangle.p1 = p1;
    rectangle.p2 = p2;
    
    return rectangle;
}

inline Rect2i new_rect2i_dim(i32 left, i32 top, i32 width, i32 height) {
    Rect2i rectangle = {};
    
    rectangle.left   = left;
    rectangle.top    = top;
    rectangle.right  = left + width - 1;
    rectangle.bottom = top + height - 1;
    
    return rectangle;
}

inline Rect2i new_rect2i_dim(Vec2i p1, Vec2i p2) {
    Rect2i rectangle = {};
    
    rectangle.p1 = p1;
    rectangle.p2 = { p1.x + p2.x - 1, p1.y + p2.y - 1 };
    
    return rectangle;
}

inline Vec2f get_rect_dimensions(Rect2f* rectangle) {
    return new_vec2f(rectangle->right - rectangle->left, rectangle->bottom - rectangle->top);
}
