#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <glm/glm.hpp>

typedef struct alignas(16) {
    alignas(16) glm::vec4 albedo;
} SimpleMaterial;

#endif
