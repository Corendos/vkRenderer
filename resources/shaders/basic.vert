#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 color;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec3 fragNormal;

layout(set = 0, binding = 0) uniform Context {
    mat4 projection;
    mat4 view;
	vec3 lightPosition;
	vec3 viewPosition;
} ctx;

layout(set = 1, binding = 0) uniform Model {
    mat4 model;
    mat4 normal;
} model;

void main() {
	vec4 worldPosition = model.model * vec4(position, 1.0);
    gl_Position = ctx.projection * ctx.view * worldPosition;
    fragColor = color;
	fragPos = vec3(worldPosition);
	fragNormal = mat3(model.normal) * normal;
}
