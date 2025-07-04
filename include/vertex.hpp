#ifndef VERTEX_H
#define VERTEX_H

#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>

typedef struct Vertex {
	glm::vec3 pos;
//	glm::vec3 nor;
//	glm::vec2 uv;
} Vertex;

Vertex triangle[3] = {{{-0.5f, -0.5f, 0.0f}}, {{0.5f, 0.5f, 0.0f}}, {{0.0f, 0.5f, 0.0f}}};
uint32_t triangle_indices[3] = {0, 1, 2};

#endif
