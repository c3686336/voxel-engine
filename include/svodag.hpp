#ifndef SVODAG_H
#define SVODAG_H

#include "common.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <queue>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <vector>

typedef uint32_t Addr_t; // Have to fix the paddings before changing this type

// Refer to the glsl std430 specification for correct padding/alignment. IDK yet
// bool, uint, int, float, double (scalars): no padding, alignment = size
// Array of scalars: no padding, alignment = size
// vec2: no padding, alignment = size
// vec3, vec4: pad to match the size of vec4, alignment = size + padding
//
typedef struct {
    glm::vec4 color; // 16 bytes
    // Read as single vec4, overall size 16B

    std::array<Addr_t, 8>
        addr; // 32B (for now)
              // One is read as a single uint, overall size 4B;

    // Total size = 48B, needs to be padded to 48B = 16B * 3
} SerializedNode;
// Good enough for now

class SvoNode;

typedef struct {
    std::shared_ptr<SvoNode> node;
    size_t at_level; // Level is maximum at root
} QueryResult;

template <typename CharT> struct std::formatter<SerializedNode, CharT> {
    template <typename FormatParseContext>
    constexpr auto parse(FormatParseContext& pc) {
        return pc.begin();
    }

    template <typename FormatContext>
    auto format(SerializedNode p, FormatContext& fc) const {
        return std::format_to(
            fc.out(),
            "{{Color: {}, Connected to: [{}, {}, {}, {}, {}, {}, {}, {}]}}\n",
            p.color, p.addr[0], p.addr[1], p.addr[2], p.addr[3], p.addr[4],
            p.addr[5], p.addr[6], p.addr[7]
        );
    }
};

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
        const size_t x_bitmask, const size_t y_bitmask, const size_t z_bitmask,
        const size_t level, const glm::vec4 new_color
    ) noexcept;
    glm::vec4
    get(const size_t x_bitmask, const size_t y_bitmask, const size_t z_bitmask,
        const size_t level) const noexcept;
    const glm::vec4 get_color() const noexcept;
    const std::array<std::shared_ptr<SvoNode>, 8> get_children() const noexcept;
    const std::optional<QueryResult> query(
        const size_t x_bitmask, const size_t y_bitmask, const size_t z_bitmask,
        const size_t level
    ) const noexcept;

    friend void swap(SvoNode& first, SvoNode& second);

private:
    glm::vec4 color;
    // std::shared_ptr<std::array<SvoNode, 8>>
    //    children; // TODO swap shared_ptr and array
    std::array<std::shared_ptr<SvoNode>, 8> children;
};

class SvoDag {
public:
    SvoDag() noexcept;
    SvoDag(size_t level) noexcept;

    void insert(const glm::vec3 pos, const glm::vec4 new_color) noexcept;
    void insert(
        const size_t x_bitmask, const size_t y_bitmask, const size_t z_bitmask,
        glm::vec4 color
    ) noexcept;
    const glm::vec4 get(const glm::vec3 pos) const noexcept;
    const glm::vec4
    get(const size_t x_bitmask, const size_t y_bitmask,
        const size_t z_bitmask) const noexcept;
    const QueryResult
    query(const glm::vec3 pos, const size_t max_level) const noexcept;
    const QueryResult query(const glm::vec3 pos) const noexcept;

    const std::vector<SerializedNode> serialize() const noexcept;
    inline const size_t get_level() const noexcept { return level; }

private:
    std::shared_ptr<SvoNode> root;
    size_t level; // *Height* of the octree
};

inline const float level_to_size(const size_t level, const size_t max_level) {
    return powf(0.5f, float(max_level - level));
}

inline const glm::vec3 snap_pos(const glm::vec3 pos, size_t level) {
    return glm::vec3(
		floorf(pos.x * (1 << level)) / pow(0.5f, level),
		floorf(pos.y * (1 << level)) / pow(0.5f, level),
		floorf(pos.y * (1 << level)) / pow(0.5f, level)
	);
}

#endif
