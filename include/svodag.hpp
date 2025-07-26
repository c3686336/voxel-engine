#ifndef SVODAG_H
#define SVODAG_H

#include "common.hpp"
#include "material_list.hpp"

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
#include <utility>

typedef uint32_t Addr_t; // Have to fix the paddings before changing this type

// Refer to the glsl std430 specification for correct padding/alignment. IDK yet
// bool, uint, int, float, double (scalars): no padding, alignment = size
// Array of scalars: no padding, alignment = size
// vec2: no padding, alignment = size
// vec3, vec4: pad to match the size of vec4, alignment = size + padding
//
typedef struct alignas(4) SerializedNode {
    alignas(sizeof(MatID_t)) MatID_t mat_id; // 4 bytes
    // Read as single vec4, overall size 16B

    alignas(4) std::array<Addr_t, 8> addr; // 4 Bytes * 8 = 32 Bytes (for now)
    // One is read as a single uint, overall size 4B;

    // Total size = 48B, needs to be padded to 48B = 16B * 3

    bool operator==(const SerializedNode& other) const = default;
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
            "{{Material id: {}, Connected to: [{}, {}, {}, {}, {}, {}, {}, "
            "{}]}}\n",
            p.mat_id, p.addr[0], p.addr[1], p.addr[2], p.addr[3], p.addr[4],
            p.addr[5], p.addr[6], p.addr[7]
        );
    }
};

template<>
struct std::hash<SvoNode> {
    std::size_t operator()(SvoNode const& node) const noexcept;
};

// Definitions
class SvoNode {
public:
    SvoNode() noexcept;
    SvoNode(MatID_t mat_id) noexcept;

    inline bool operator==(const SvoNode& other) const = default;

    void insert(
        const size_t x_bitmask, const size_t y_bitmask, const size_t z_bitmask,
        const size_t level, const MatID_t new_mat_id
    ) noexcept;
    MatID_t
    get(const size_t x_bitmask, const size_t y_bitmask, const size_t z_bitmask,
        const size_t level) const noexcept;
    MatID_t get_mat_id() const noexcept;
    const std::array<std::shared_ptr<SvoNode>, 8> get_children() const noexcept;
    const std::optional<QueryResult> query(
        const size_t x_bitmask, const size_t y_bitmask, const size_t z_bitmask,
        const size_t level
    ) const noexcept;

    void dedup(std::unordered_map<SvoNode, std::shared_ptr<SvoNode>>& map, size_t target_depth /*Opposite of level*/);
    void solidify_tree();
    void solidify_this();

    friend void swap(SvoNode& first, SvoNode& second);

    friend size_t std::hash<SvoNode>::operator()(const SvoNode& node) const noexcept;

private:
    MatID_t mat_id;
    // std::shared_ptr<std::array<SvoNode, 8>>
    //    children; // TODO swap shared_ptr and array
    std::array<std::shared_ptr<SvoNode>, 8> children;
};

class SvoDag {
public:
    SvoDag() noexcept;
    SvoDag(size_t level) noexcept;

    void insert(const glm::vec3 pos, const MatID_t new_mat_id) noexcept;
    void insert(
        const size_t x_bitmask, const size_t y_bitmask, const size_t z_bitmask,
        MatID_t mat_id 
    ) noexcept;
    MatID_t get(const glm::vec3 pos) const noexcept;
    MatID_t
    get(const size_t x_bitmask, const size_t y_bitmask,
        const size_t z_bitmask) const noexcept;
    const QueryResult
    query(const glm::vec3 pos, const size_t max_level) const noexcept;
    const QueryResult query(const glm::vec3 pos) const noexcept;

    const std::vector<SerializedNode> serialize() const noexcept;
    inline size_t get_level() const noexcept { return level; }

    void dedup() noexcept;

private:
    std::shared_ptr<SvoNode> root;
    size_t level; // *Height* of the octree
};

inline float level_to_size(const size_t level, const size_t max_level) {
    return powf(0.5f, (float)(max_level - level));
}

inline const glm::vec3
snap_pos(const glm::vec3 pos, size_t level, size_t max_level) {
    return glm::vec3(
        floorf(pos.x * float(1 << (max_level - level))) *
            powf(0.5f, (float)(max_level - level)),
        floorf(pos.y * float(1 << (max_level - level))) *
            powf(0.5f, (float)(max_level - level)),
        floorf(pos.z * float(1 << (max_level - level))) *
            powf(0.5f, (float)(max_level - level))
    );
}

inline const glm::vec3
snap_pos_up(const glm::vec3 pos, size_t level, size_t max_level) {
    return glm::vec3(
        ceilf(pos.x * float(1 << (max_level - level))) *
            powf(0.5f, (float)(max_level - level)),
        ceilf(pos.y * float(1 << (max_level - level))) *
            powf(0.5f, (float)(max_level - level)),
        ceilf(pos.z * float(1 << (max_level - level))) *
            powf(0.5f, (float)(max_level - level))
    );
}

std::tuple<size_t, size_t, size_t>
pos_to_bitmask(const glm::vec3 pos, size_t level) noexcept;

#endif
