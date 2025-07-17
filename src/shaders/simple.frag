#version 450 core

layout (location = 0) in vec3 color;

<<<<<<< Updated upstream
layout(std430, binding = 3) buffer asdf {
    int data[];
};

=======
<<<<<<< Updated upstream
=======
struct Node {
    vec4 color;
    uint addr[8];
};

layout(std430, binding = 3) buffer asdf {
    Node nodes[];
};

>>>>>>> Stashed changes
>>>>>>> Stashed changes
out vec4 fragcolor;

void main() {
	fragcolor = vec4(0.5f * (color.rg + vec2(1.0f)), 0.0f, 1.0f);
}
