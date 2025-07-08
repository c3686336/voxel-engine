#include "svodag.hpp"

size_t bitmask_to_index(const size_t x_bitmask, const size_t y_bitmask, const size_t z_bitmask, size_t level) {
    size_t index = ((x_bitmask & (0b1 << (level - 1))) >> (level - 1) << 2) +
                   ((y_bitmask & (0b1 << (level - 1))) >> (level - 1) << 1) +
                   ((z_bitmask & (0b1 << (level - 1))) >> (level - 1));
#ifdef DEBUG
    if (index >= 8) {
        SPDLOG_CRIT("The index calculation is probably wrong. the value = {}", index);
    }
#endif

    return index;
}

// Implementations
SvoNode::SvoNode() noexcept : SvoNode(glm::vec4(0.0f)){};

SvoNode::SvoNode(glm::vec4 new_color) noexcept
    : color(std::move(new_color)), children(){};

SvoNode::SvoNode(const SvoNode& other) noexcept : color(other.color) {
    for (int i = 0; i < 8; i++) {
        // Refcount is decremented
        children[i] = std::make_shared<SvoNode>(*other.children[i]);
    }
}

SvoNode::SvoNode(SvoNode&& other) noexcept
    : color(std::move(other.color)), children(std::move(other.children)) {
    other.children =
        std::array<std::shared_ptr<SvoNode>, 8>(); // So that the dtor is no-op.
};

SvoNode& SvoNode::operator=(SvoNode other) noexcept {
    swap(*this, other);

    return *this;
}

SvoNode& SvoNode::operator=(SvoNode&& other) noexcept {
    color = std::move(other.color);
    children = std::move(other.children);
    other.children =
        std::array<std::shared_ptr<SvoNode>, 8>(); // So that the dtor is no-op.

    return *this;
}

void SvoNode::insert(
    const size_t x_bitmask, const size_t y_bitmask, const size_t z_bitmask,
    const int level, const glm::vec4 new_color
) noexcept {
    // TODO: Needs significant refactoring if the structure is DAG. Just
    // allocate new node path then. shared_ptr will handle the rest.

    if (level == 0) {
        this->color = std::move(new_color);
        return;
    }

    size_t index = bitmask_to_index(x_bitmask, y_bitmask, z_bitmask, level);

    if (!children[index]) {
        // Allocate them
        children[index] = std::make_shared<SvoNode>();
    }

    children.at(index)->insert(
        std::move(x_bitmask), std::move(y_bitmask), std::move(z_bitmask),
        level - 1, std::move(new_color)
    );

    // Also reset the color;
    color = glm::vec4(0.0f);
    for (int i = 0; i < 8; i++) {
        if (children[i]) {
            color += children[i]->color / 8.0f;
        }
    }
}

glm::vec4 SvoNode::get(
    size_t x_bitmask, size_t y_bitmask, size_t z_bitmask, int level
) const noexcept {
    if (level == 0) {
        return color;
    }

    size_t index = bitmask_to_index(x_bitmask, y_bitmask, z_bitmask, level);

    if (!children[index]) {
        return color;
    } else {
        return get(x_bitmask, y_bitmask, z_bitmask, level - 1);
    }
}

const glm::vec4 SvoNode::get_color() const noexcept { return color; }

void swap(SvoNode& first, SvoNode& second) {
    using std::swap;

    std::swap(first.color, second.color);
    std::swap(first.children, second.children);
}

const std::array<std::shared_ptr<SvoNode>, 8>
SvoNode::get_children() const noexcept {
    return children;
}

SvoDag::SvoDag() noexcept : root(), level(8 /*2^8^3 = 256^3 voxels*/){};
void SvoDag::insert(glm::vec3 pos, glm::vec4 color) noexcept {
    size_t x_bitmask = pos.x * (1 << level);
    size_t y_bitmask = pos.y * (1 << level);
    size_t z_bitmask = pos.z * (1 << level);

    root.insert(x_bitmask, y_bitmask, z_bitmask, level, color);
};

const std::vector<SerializedNode> SvoDag::serialize() const noexcept {
    // TODO;
    // Traverse the tree(dag) in a breath-first order, make an array, use
    // pointer arithmetic to retrieve the index Write it

    std::vector<SerializedNode> buffer;

    std::unordered_set<std::shared_ptr<SvoNode>> visited;
    std::queue<std::shared_ptr<SvoNode>> to_visit;

    buffer.push_back(
        {glm::vec4(0.0f),
         {
             0,
         }}
    );

    buffer.push_back(
        {root.get_color(),
         {
             0,
         }}
    );

    int cnt = 1;
    for (int i = 0; i < 8; i++) {
        if (root.get_children()[i]) {
            buffer.back().addr[i] = cnt;
            cnt++;
            to_visit.push(root.get_children()[i]);
        } else {
            buffer.back().addr[i] = 0;
        }
    }

    while (!to_visit.empty()) {
        auto node = to_visit.back();
        to_visit.pop();

        if (visited.contains(node)) {
            continue;
        }

        visited.insert(node);

        for (int i = 0; i < 8; i++) {
            if (node->get_children()[i]) {
                buffer.back().addr[i] = buffer.size();
                to_visit.push(node->get_children()[i]);
                // This will work because the queue is sorta like a view?
            } else {
                buffer.back().addr[i] = 0;
            }
        }
    }

    return buffer;
}

const glm::vec4 SvoDag::get(glm::vec3 pos) const noexcept {
    size_t x_bitmask = pos.x * (1 << level);
    size_t y_bitmask = pos.y * (1 << level);
    size_t z_bitmask = pos.z * (1 << level);

    return root.get(x_bitmask, y_bitmask, z_bitmask, level);
}
