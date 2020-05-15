#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;
layout(location = 3) in float textBlend;
layout(location = 4) in uint fontIndex;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fraguv;
layout(location = 2) out float fragTextBlend;
layout(location = 3) out uint fragFontIndex;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    fragColor = color;
	fraguv = uv;
	fragTextBlend = textBlend;
	fragFontIndex = fontIndex;
}
