#ifndef __CG_VERTEX_H__
#define __CG_VERTEX_H__

#include "cg_math.h"

struct Vertex {
    Vec3f position;
    Vec2f uv;
    Vec3f normal;
    Vec3f color;
};


Vertex make_vertex(Vec3f position);
Vertex make_vertex(Vec3f position, Vec2f uv);
Vertex make_vertex(Vec3f position, Vec2f uv, Vec3f normal);
Vertex make_vertex(Vec3f position, Vec3f normal);
Vertex make_vertex(Vec3f position, Vec2f uv, Vec3f normal, Vec3f color);
Vertex make_vertex_color(Vec3f position, Vec3f color);

#endif //CG_VERTEX_H
