#ifndef __COLOR_H__
#define __COLOR_H__

#include "math.h"

Vec4f new_colorf(f32 r, f32 g, f32 b, f32 a = 1.0f);
Vec4f new_coloru(u8 r, u8 g, u8 b, u8 a = 255);

Vec4f random_color();

#endif