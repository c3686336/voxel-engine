#ifndef VERTEX_H
#define VERTEX_H

#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>

typedef struct Vertex {
	glm::vec3 pos;
//	glm::vec3 nor;
//	glm::vec2 uv;
} Vertex;

const Vertex fullscreen_quad[] = {
	{{-1.0f, -1.0f, 0.0f}},
	{{3.0f, -1.0f, 0.0f}},
	{{-1.0f, 3.0f, 0.0f}}
};

const uint32_t fullscreen_indices[] = {0, 1, 2};

#endif
