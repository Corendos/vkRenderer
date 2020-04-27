Vec4f new_colorf(float r, float g, float b, float a) {
    return new_vec4f(r, g, b, a);
}

Vec4f new_coloru(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return new_colorf(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}