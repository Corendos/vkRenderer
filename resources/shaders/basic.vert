#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 fragColor;

layout(set = 0, binding = 0) uniform Context {
    mat4 projection;
    mat4 view;
} ctx;

layout(set = 1, binding = 0) uniform Model {
    mat4 matrix;
} model;

void main() {
    gl_Position = ctx.projection * ctx.view * model.matrix * vec4(position, 1.0);
    fragColor = color;
}
