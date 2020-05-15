#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fraguv;
layout(location = 2) in float fragTextBlend;
layout(location = 3) flat in uint fragFontIndex;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2DArray guiSampler;

void main() {
	vec4 textColor = vec4(fragColor.rgb, texture(guiSampler, vec3(fraguv, fragFontIndex)).r * fragColor.a);
    outColor = (1.0 - fragTextBlend) * fragColor + fragTextBlend * textColor;
}
