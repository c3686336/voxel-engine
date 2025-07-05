#version 450 core

layout (location = 1) in vec3 color;
out vec4 fragcolor;

void main() {
	fragcolor = vec4(0.5f * (color.rg + vec2(1.0f)), 0.0f, 1.0f);
}
