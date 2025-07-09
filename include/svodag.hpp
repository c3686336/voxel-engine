#ifndef SVODAG_H
#define SVODAG_H

#include "common.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <queue>
#include <ranges>
#include <unordered_set>
#include <unordered_map>
#include <vector>

typedef uint16_t Addr_t; // Have to fix the paddings before changing this type

// Refer to the glsl std430 specification for correct padding/alignment. IDK yet
// bool, uint, int, float, double (scalars): no padding, alignment = size
// Array of scalars: no padding, alignment = size
// vec2: no padding, alignment = size
// vec3, vec4: pad to match the size of vec4, alignment = size + padding
//
typedef struct {
    glm::vec4 color; // 16 bytes
    // Read as single vec4, overall size 16B

    std::array<Addr_t, 8> addr; // 16B (for now)
                           // Two are read as single uint, overall size 4B;

    // Total size = 32B, needs to be padded to 32B = 16B * 3
} SerializedNode;
// Good enough for now

// Definitions
class SvoNode {
public:
    SvoNode() noexcept;
    SvoNode(glm::vec4 new_color) noexcept;
    SvoNode(const SvoNode& other) noexcept;
    SvoNode(SvoNode&& other) noexcept;
    SvoNode& operator=(SvoNode other) noexcept;
    SvoNode& operator=(SvoNode&& other) noexcept;

    void insert(
        size_t x_bitmask, size_t y_bitmask, size_t z_bitmask, int level,
        glm::vec4 color
    ) noexcept;
	glm::vec4 get(
        size_t x_bitmask, size_t y_bitmask, size_t z_bitmask, int level
    ) const noexcept;
    const glm::vec4 get_color() const noexcept;
    const std::array<std::shared_ptr<SvoNode>, 8> get_children() const noexcept;

    friend void swap(SvoNode& first, SvoNode& second);

    glm::vec4 color;
    // std::shared_ptr<std::array<SvoNode, 8>>
    //    children; // TODO swap shared_ptr and array
    std::array<std::shared_ptr<SvoNode>, 8> children;
};

class SvoDag {
public:
    SvoDag() noexcept;

    void insert(glm::vec3 pos, glm::vec4 new_color) noexcept;
    const glm::vec4 get(glm::vec3 pos) const noexcept;

    const std::vector<SerializedNode> serialize() const noexcept;

private:
    SvoNode root;
    int level; // *Height* of the octree
};

#endif
