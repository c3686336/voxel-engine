#include "formatter.hpp"
#include "svodag.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <glm/glm.hpp>
#include <optional>
#include <spdlog/spdlog.h>

typedef struct {
    glm::vec3 origin;
    glm::vec3 dir; // Must be normalized
} Ray;

std::byte float_to_255(const float float_color) {
    return static_cast<std::byte>(
        std::floorf(float_color == 1.0f ? 255 : float_color * 256.0f)
    );
}

std::array<std::byte, 4> float_to_255(const glm::vec4 float_color) {
    return {
        float_to_255(float_color.r), float_to_255(float_color.g),
        float_to_255(float_color.b), float_to_255(float_color.a)
    };
}

glm::vec4 /*Voxel Color*/
raymarch(const SvoDag& svodag, const Ray ray) {
    // This is only an example implementation

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
        return glm::vec4(0.0f);
    }

    // Raymarch up to the aabb of the svodag
    glm::vec3 pos = ray.origin + ray.dir * tmin;

    // loop:
    // If the node is air (alpha is zero), raymarch to the end of current node
    // End loop

    while (pos.x >= 0.0f && pos.y >= 0.0f && pos.z >= 0.0f && pos.x < 1.0f &&
           pos.y < 1.0f && pos.z < 1.0f) {
        // Get the deepest node that intersects the point
        QueryResult result = svodag.query(pos); // Without bias as first test

        if (result.node->get_color().a > 0.0f) {
            // This currently does not support alpha blending
            auto color = result.node->get_color();
            // return glm::vec4(color.r, color.g, color.b, 1.0f);
            return color;
        }

        // Have to find the position of the voxel
        float size = level_to_size(result.at_level, level);
        glm::vec3 voxel_end = snap_pos(pos, level) + glm::vec3(size);

        glm::vec3 tvec = (voxel_end - pos) * dir_inv;
        float tmax = std::max(tvec.x, std::max(tvec.y, tvec.z));

        pos += ray.dir * tmax;

        // Find next intersection
    }

    return glm::vec4(0.0f);
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
                if (16000 < length && length <= 16385) {
                    svodag.insert(
                        x, y, z,
                        glm::vec4(
                            (float)x / 256.0f, (float)y / 256.0f,
                            (float)z / 256.0f, 1.0f
                        )
                    );
                }
            }
        }
    }
    SPDLOG_INFO("Constructed SvoDag");

    auto result = raymarch(
        svodag, {glm::vec3(-1.0f, 0.5f, 0.5f),
                 glm::normalize(
                     glm::vec3(0.998553097f, -0.0199710727f, -0.0499276668f)
                 )}
    );

    SPDLOG_INFO(std::format("{}", result));

    SPDLOG_INFO("Raymarching...");
    const int width = 100;
    const int height = 100;

    const float film_width = 1.0f;
    const float film_height = 1.0f;
    const float film_offset = 1.0f; // Corresponds to FOV

    std::vector<std::array<std::byte, 4>> data{};
    data.reserve(width * height);

    // stb wants first raw, then second row...
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            // SPDLOG_INFO("x: {} y: {}", j, i);

            glm::vec3 origin = glm::vec3(-film_offset, 0.5f, 0.5f);
            glm::vec3 dir = glm::normalize(
                glm::vec3(
                    film_offset,
                    j / static_cast<float>(width) * film_width -
                        film_width / 2.0f,
                    i / static_cast<float>(height) * film_height -
                        film_height / 2.0f
                )
            );

            // SPDLOG_INFO(std::format("{} {}", origin, dir));

            data.push_back(float_to_255(raymarch(svodag, {origin, dir})));
        }
    }

    SPDLOG_INFO("Done!");

    stbi_write_png("asdf.png", width, height, 4, data.data(), width * 4);

    // SPDLOG_INFO(std::format("{}", raymarch(svodag, {glm::vec3(-1.0f, 0.5f,
    // 0.5f), glm::vec3(1.0f, 0.0f, 0.0f)}).value()));

    return 0;
}
