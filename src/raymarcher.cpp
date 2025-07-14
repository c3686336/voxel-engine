#include "svodag.hpp"
#include "formatter.hpp"

#include <glm/glm.hpp>
#include <optional>
#include <spdlog/spdlog.h>

typedef struct {
    glm::vec3 origin;
    glm::vec3 dir; // Must be normalized
} Ray;

std::optional<glm::vec3> /*Hit position*/
raymarch(const SvoDag& svodag, const Ray ray) {
    const glm::vec3 bias = ray.dir * level_to_size(svodag.get_level(), 0) /
                           4.0f; // A small bias for ensuring that the ray's
                                 // endpoint always intersects a voxel

    /*Assume that the svodag always spans (0, 0, 0) ~ (1, 1, 1)*/
    size_t level = svodag.get_level();
    glm::vec3 dir_inv =
        glm::vec3(1.0 / ray.dir.x, 1.0 / ray.dir.y, 1.0 / ray.dir.z);

    // Ray-AABB Intersection
    // TODO: Separate this out into another function
    // Branchless Ray-AABB intersection by
    // https://tavinator.com/2011/ray_box.html
    float tx1 = -ray.origin.x * dir_inv.x; // Handles dir.x == 0 case
    float tx2 = (1 - ray.origin.x) * dir_inv.x;

    float tmin = std::min(tx1, tx2);
    float tmax = std::min(tx1, tx2);

    float ty1 = -ray.origin.y * dir_inv.y; // ,,
    float ty2 = (1 - ray.origin.y) * dir_inv.y;

    tmin = std::min(std::max(ty1, ty2), tmin);
    tmax = std::max(std::min(ty1, ty2), tmax);

    float tz1 = -ray.origin.z * dir_inv.z; // ,,
    float tz2 = (1 - ray.origin.z) * dir_inv.z;

    tmin = std::min(std::max(tz1, tz2), tmin);
    tmax = std::max(std::min(tz1, tz2), tmax);

    bool intersected = (tmax >= tmin) && tmin >= 0.f;

    // If the ray is outside of the svodag,
    if (!intersected) {
        return std::nullopt;
    }

    // Raymarch up to the aabb of the svodag
    glm::vec3 pos = ray.origin + ray.dir * tmin;

    // loop:
    // If the node is air (alpha is zero), raymarch to the end of current node
    // End loop

    while (pos.x >= 0.0f && pos.y >= 0.0f,
           pos.z >= 0.0f && pos.x < 1.0f && pos.y < 1.0f && pos.z < 1.0f) {
        // Go the the deepest node that intersects the point
        QueryResult result =
            svodag.query(pos /*+ bias*/); // Without bias as first test

        if (result.node->get_color().a > 0.0f) {
            return result.node->get_color();
        }

        // Have to find the position of the voxel
        float size = level_to_size(result.at_level, level);
        glm::vec3 voxel_end = snap_pos(pos, level) + glm::vec3(size);

        glm::vec3 tvec = (voxel_end - ray.origin) * dir_inv;
        float tmax = std::max(
            tvec.x, std::max(tvec.y, tvec.z)
        ); // From the original origin of the ray, not from `pos`

        pos = ray.origin + ray.dir * tmax;

        // Find next intersection
    }

	return std::nullopt;
}


int main(int argc, char** argv) {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%@] %v");
    SPDLOG_INFO("Program Started");

    SPDLOG_INFO("Constructing SvoDag");
    SvoDag svodag{8}; // width = 256;

    for (size_t x = 0; x < 256; x++) {
        for (size_t y = 0; y < 256; y++) {
            for (size_t z = 0; z < 256; z++) {
                size_t length = (x - 128) * (x - 128) + (y - 128) * (y - 128) +
                                (z - 128) * (z - 128);
                if (16383 < length && length < 16385) {
                    svodag.insert(
                        x, y, z,
                        glm::vec4(
                            (float)x / 256.0f, (float)y / 256.0f, z / 256.0f,
                            1.0f
                        )
                    );
                }
            }
        }
    }
    SPDLOG_INFO("Constructed SvoDag");

	SPDLOG_INFO("Raymarching...");

	SPDLOG_INFO(std::format("{}", raymarch(svodag, {glm::vec3(-1.0f, 0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f)}).value()));

    return 0;
}

