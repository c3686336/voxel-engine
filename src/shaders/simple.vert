#version 450 core

layout (location = 0) in vec3 pos;
layout (location = 0) out vec2 frag_pos;

void main() {
	frag_pos = pos.xy;
	gl_Position = vec4(pos, 1.0f);
}

