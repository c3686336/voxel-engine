#version 450 core

layout(location = 0) in vec2 frag_pos;

layout(location = 1) uniform sampler2D in_img;

out vec4 frag_color;

void main() {
    frag_color = texture(in_img, (frag_pos + 1.0) / 2.0);
}
