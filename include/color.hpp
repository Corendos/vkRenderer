#ifndef __COLOR_HPP__
#define __COLOR_HPP__

#include "math.hpp"

Vec4f new_colorf(float r, float g, float b, float a = 1.0f);
Vec4f new_coloru(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

#endif