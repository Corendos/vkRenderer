#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform Context {
    mat4 projection;
    mat4 view;
	vec3 lightPosition;
	vec3 viewPosition;
} ctx;


void main() {
	vec3 lightColor = vec3(1.0, 1.0, 1.0);
	vec3 ambientColor = lightColor * 0.1;

	vec3 norm = normalize(fragNormal);
	vec3 lightDir = normalize(ctx.lightPosition - fragPos);
	float diff = max(dot(norm, lightDir), 0.0);

	vec3 diffuseColor = diff * lightColor;

    outColor = vec4((ambientColor + diffuseColor) * fragColor, 1.0);
}
