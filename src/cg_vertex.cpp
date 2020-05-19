#include "cg_vertex.h"

#include "cg_math.h"
#include "cg_color.h"

inline Vertex make_vertex(Vec3f position) {
    return make_vertex(position, new_vec2f(), new_vec3f(), new_vec3f(1.0f, 1.0f, 1.0f));
}

inline Vertex make_vertex(Vec3f position, Vec2f uv) {
    return make_vertex(position, uv, new_vec3f(), new_vec3f(1.0f, 1.0f, 1.0f));
}

inline Vertex make_vertex(Vec3f position, Vec2f uv, Vec3f normal) {
    return make_vertex(position, uv, normal, new_vec3f(1.0f, 1.0f, 1.0f));
}

inline Vertex make_vertex(Vec3f position, Vec3f normal) {
    return make_vertex(position, new_vec2f(), normal, new_vec3f(1.0f, 1.0f, 1.0f));
}

inline Vertex make_vertex_color(Vec3f position, Vec3f color) {
    return make_vertex(position, new_vec2f(), new_vec3f(), color);
}

inline Vertex make_vertex(Vec3f position, Vec2f uv, Vec3f normal, Vec3f color) {
    Vertex vertex = {};
    vertex.position = position;
    vertex.uv       = uv;
    vertex.normal   = normal;
    vertex.color    = color;
    return vertex;
}
