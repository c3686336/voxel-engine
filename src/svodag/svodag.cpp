#include "svodag.hpp"
#include "formatter.hpp"
#include "spdlog/spdlog.h"

#include <utility>

size_t bitmask_to_index(
    const size_t x_bitmask, const size_t y_bitmask, const size_t z_bitmask,
    size_t level
) {
    size_t index = (((x_bitmask >> (level - 1)) & 0b1) << 2) +
                   (((y_bitmask >> (level - 1)) & 0b1) << 1) +
                   (((z_bitmask >> (level - 1)) & 0b1) << 0);

    if (index >= 8) {
        SPDLOG_CRITICAL(
            "The index calculation is probably wrong. the value = {}", index
        );
    }

    return index;
}

// Implementations
SvoNode::SvoNode() noexcept : SvoNode(0){};

SvoNode::SvoNode(MatID_t mat_id) noexcept
    : mat_id(mat_id), children(){};

SvoNode::SvoNode(const SvoNode& other) noexcept : mat_id(other.mat_id) {
    for (int i = 0; i < 8; i++) {
        // Refcount is decremented
        children[i] = std::make_shared<SvoNode>(*other.children[i]);
    }
}

SvoNode::SvoNode(SvoNode&& other) noexcept
    : mat_id(other.mat_id), children(std::move(other.children)) {
    other.children =
        std::array<std::shared_ptr<SvoNode>, 8>(); // So that the dtor is no-op.
};

SvoNode& SvoNode::operator=(SvoNode other) noexcept {
    swap(*this, other);

    return *this;
}

SvoNode& SvoNode::operator=(SvoNode&& other) noexcept {
    mat_id = std::move(other.mat_id);
    children = std::move(other.children);
    other.children =
        std::array<std::shared_ptr<SvoNode>, 8>(); // So that the dtor is no-op.

    return *this;
}

void SvoNode::insert(
    const size_t x_bitmask, const size_t y_bitmask, const size_t z_bitmask,
    const size_t level, const MatID_t new_mat_id
) noexcept {
    // TODO: Needs significant refactoring if the structure is DAG. Just
    // allocate new node path then. shared_ptr will handle the rest.

    if (level == 0) {
        this->mat_id = new_mat_id;
        return;
    }

    size_t index = bitmask_to_index(x_bitmask, y_bitmask, z_bitmask, level);

    if (!children[index]) {
        // Allocate them
		// New Semantic: All nodes have either 8 or 0 children.
		for (int i=0;i<8;i++) {
			assert(children[i] == nullptr); // Guaruntee: All nodes must have 8 children or not at all. 
			children[i] = std::make_shared<SvoNode>(); // Maybe explicitely set the color here?
		}
        // children[index] = std::make_shared<SvoNode>();
    }
#ifdef DAG
    // If the node has been de-duped
    // This will create memory overhead if there is any external reference to this node;
    // Unlikely. However, be careful not to call this function while serializing
    // or dedupping
    if (children[index].use_count() != 1) {
        // deep-copy the node
        children[index] = std::make_shared<SvoNode>(*children[index]);
    }
#endif

    children.at(index)->insert(
        std::move(x_bitmask), std::move(y_bitmask), std::move(z_bitmask),
        level - 1, new_mat_id
    );

    // Also reset the color;
    mat_id = 0;
    // TODO: Store average albedo directly
    // color = glm::vec4(0.0f);
    // for (int i = 0; i < 8; i++) {
    //     if (children[i]) {
    //         color += children[i]->color / 8.0f;
    //     }
    // }
}

MatID_t SvoNode::get(
    size_t x_bitmask, size_t y_bitmask, size_t z_bitmask, size_t level
) const noexcept {
    if (level == 0) {
        return mat_id;
    }

    size_t index = bitmask_to_index(x_bitmask, y_bitmask, z_bitmask, level);

    if (!children[index]) {
        return mat_id;
    } else {
        return children.at(index)->get(
            x_bitmask, y_bitmask, z_bitmask, level - 1
        );
    }
}

MatID_t SvoNode::get_mat_id() const noexcept { return mat_id; }

void swap(SvoNode& first, SvoNode& second) {
    using std::swap;

    swap(first.mat_id, second.mat_id);
    swap(first.children, second.children);
}

const std::array<std::shared_ptr<SvoNode>, 8>
SvoNode::get_children() const noexcept {
    return children;
}

const std::optional<QueryResult> SvoNode::query(
    const size_t x_bitmask, const size_t y_bitmask, const size_t z_bitmask,
    const size_t level
) const noexcept {
    // Returns std::nullopt if the shared_ptr of this has to be returned
    if (level == 0) {
        return std::nullopt;
    }

    size_t index = bitmask_to_index(x_bitmask, y_bitmask, z_bitmask, level);

    if (children[index]) {
        auto result =
            children[index]->query(x_bitmask, y_bitmask, z_bitmask, level - 1);

        if (result.has_value()) {
            return result;
        } else {
            return std::make_optional<QueryResult>(children[index], level - 1);
        }

    } else {
		// Same here: If there are no children (As guaranteed), return this.
        return std::nullopt;
    }
}

SvoDag::SvoDag() noexcept : root(std::make_shared<SvoNode>()), level(8 /*2^8^3 = 256^3 voxels*/) {};
SvoDag::SvoDag(size_t level) noexcept : root(std::make_shared<SvoNode>()), level(level) {};

void SvoDag::insert(const glm::vec3 pos, const MatID_t mat_id) noexcept {
    size_t x_bitmask = pos.x * (1 << level);
    size_t y_bitmask = pos.y * (1 << level);
    size_t z_bitmask = pos.z * (1 << level);

    insert(x_bitmask, y_bitmask, z_bitmask, mat_id);
};

void SvoDag::insert(
    const size_t x_bitmask, const size_t y_bitmask, const size_t z_bitmask,
    const MatID_t mat_id
) noexcept {
    root->insert(x_bitmask, y_bitmask, z_bitmask, level, mat_id);
}

const std::vector<SerializedNode> SvoDag::serialize() const noexcept {
    // TODO: This probably violates SRP

    Addr_t cnt = 0; // Because the root node won't be included;
    std::unordered_map<std::shared_ptr<SvoNode>, Addr_t> map;
    std::queue<std::shared_ptr<SvoNode>> bfs_queue;

    cnt++;
    map[root] = cnt;
    bfs_queue.push(root);

    while (!bfs_queue.empty()) {
        auto node = bfs_queue.front();
        bfs_queue.pop();

        for (int i = 0; i < 8; i++) {
            if (node->get_children()[i] &&
                !map.contains(node->get_children()[i])) {
                cnt++;
                map[node->get_children()[i]] = cnt;

                bfs_queue.push(node->get_children()[i]);
            }
        }
    }

    // Now, cnt is the number of nodes
    std::vector<SerializedNode> buffer(cnt + 1, SerializedNode{});

    SPDLOG_INFO(cnt);

    buffer[1] = SerializedNode{root->get_mat_id(), std::array<Addr_t, 8>{0}};
    for (int i = 0; i < 8; i++) {
        if (root->get_children()[i]) {
            buffer[1].addr[i] = map[root->get_children()[i]];
        }
    }

    for (auto i = map.begin(); i != map.end(); i++) {
        // SPDLOG_INFO(std::format("{}", i->second));

        buffer[i->second] =
            SerializedNode{i->first->get_mat_id(), std::array<Addr_t, 8>{0}};

        for (int j = 0; j < 8; j++) {
            if (i->first->get_children()[j]) {
                buffer[i->second].addr[j] = map[i->first->get_children()[j]];
            }
        }
    }

    return buffer;
}

std::tuple<size_t, size_t, size_t> pos_to_bitmask(const glm::vec3 pos, size_t level) noexcept {
	return std::make_tuple<size_t, size_t, size_t>(
		pos.x * (1 << level),
		pos.y * (1 << level),
		pos.z * (1 << level)
	);
}

MatID_t SvoDag::get(const glm::vec3 pos) const noexcept {
	auto bitmask = pos_to_bitmask(pos, level);

	size_t x_bitmask = std::get<0>(bitmask);
	size_t y_bitmask = std::get<1>(bitmask);
	size_t z_bitmask = std::get<2>(bitmask);

    return get(x_bitmask, y_bitmask, z_bitmask);
}

MatID_t SvoDag::get(
    const size_t x_bitmask, const size_t y_bitmask, const size_t z_bitmask
) const noexcept {
    return root->get(x_bitmask, y_bitmask, z_bitmask, level);
}


const QueryResult SvoDag::query(const glm::vec3 pos) const noexcept {
	auto bitmask = pos_to_bitmask(pos, level);

	size_t x_bitmask = std::get<0>(bitmask);
	size_t y_bitmask = std::get<1>(bitmask);
	size_t z_bitmask = std::get<2>(bitmask);

	auto result = root->query(x_bitmask, y_bitmask, z_bitmask, level);
	// TODO: In order to support max_level, SvoNode::query needs to be refactored

    return result.value_or({root, level});
}
