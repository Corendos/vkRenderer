#include "math.h"

inline Vec4f new_colorf(f32 r, f32 g, f32 b, f32 a) {
    return new_vec4f(r, g, b, a);
}

inline Vec4f new_coloru(u8 r, u8 g, u8 b, u8 a) {
    return new_colorf(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

inline Vec4f random_color() {
    return new_colorf(randf() / 2.0f + 0.5f, randf() / 2.0f + 0.5f, randf() / 2.0f + 0.5f);
}