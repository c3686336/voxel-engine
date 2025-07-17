#version 450 core

layout (location = 0) in vec3 color;

struct Node {
    vec4 color;
    uint addr[8];
};

layout(std430, binding = 3) buffer asdf {
    Node nodes[];
};

out vec4 fragcolor;

void main() {
	fragcolor = vec4(0.5f * (color.rg + vec2(1.0f)), 0.0f, 1.0f);
}
