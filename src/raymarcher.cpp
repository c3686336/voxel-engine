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

inline std::pair<float, float> slab_test(
    const glm::vec3 cor1, const glm::vec3 cor2, const glm::vec3 pos,
    const glm::vec3 dir_inv
) {
    using std::min, std::max;

    // https://tavianator.com/2015/ray_box_nan.html
    glm::vec3 t1 = (cor1 - pos) * dir_inv;
    glm::vec3 t2 = (cor2 - pos) * dir_inv;

    float tmin = min(t1.x, t2.x);
    float tmax = max(t1.x, t2.x);

    // Eliminates NaN problem
    tmin = max(tmin, min(min(t1.y, t2.y), tmax));
    tmax = min(tmax, max(max(t1.y, t2.y), tmin));

    tmin = max(tmin, min(min(t1.z, t2.z), tmax));
    tmax = min(tmax, max(max(t1.z, t2.z), tmin));

    return std::make_pair(tmin, tmax);
}

glm::vec4 /*Voxel Color*/
raymarch(const SvoDag& svodag, const Ray ray) {
#ifndef NDEBUG
    static int invocation_id = 0;
    invocation_id++;
#endif

    // This is only an example implementation

    // endpoint always intersects a voxel

    /*Assume that the svodag always spans (0, 0, 0) ~ (1, 1, 1)*/
    size_t level = svodag.get_level();
    glm::vec3 dir_inv(1.0 / ray.dir.x, 1.0 / ray.dir.y, 1.0 / ray.dir.z);
    glm::vec3 dir_sign(
        ray.dir.x >= 0 ? 1 : -1, ray.dir.y >= 0 ? 1 : -1,
        ray.dir.z >= 0 ? 1 : -1
    );

    auto [tmin, tmax] = slab_test(glm::vec3(0.0f), glm::vec3(1.0f), ray.origin, dir_inv);
    
    tmin = std::max(0.0f, tmin);

    bool intersected = (tmax > tmin);

    // If the ray is outside of the svodag,
    if (!intersected) {
        return glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    // Raymarch up to the aabb of the svodag
    glm::vec3 current_pos = ray.origin + ray.dir * tmin;

    // loop:
    // If the node is air (alpha is zero), raymarch to the end of current node
    // End loop

#ifndef NDEBUG
    int iterations = 0;
#endif

    do {
        // Get the deepest node that intersects the point
        glm::vec3 bias = ray.dir * level_to_size(0, 8) * 0.01f;
        // current_pos += bias;
        QueryResult result =
            svodag.query(current_pos + bias); // Without bias as first test

        if (result.node->get_color().a > 0.0f) {
            // This currently does not support alpha blending
            auto color = result.node->get_color();
            // return glm::vec4(color.r, color.g, color.b, 1.0f);
            return color;
            // return glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        }

        // Have to find the position of the voxel

        float size = level_to_size(result.at_level, level);

        // glm::vec3 biased_pos = current_pos + size*0.1f*ray.dir;
        glm::vec3 current_voxel_start = snap_pos(current_pos + bias, level);
        glm::vec3 sign(
            current_voxel_start.x == current_pos.x ? dir_sign.x : +1.f,
            current_voxel_start.y == current_pos.y ? dir_sign.y : +1.f,
            current_voxel_start.z == current_pos.z ? dir_sign.z : +1.f
        );

        // SPDLOG_INFO(std::format("{}", sign));

        // sign = glm::vec3(1.f, 1.f, 1.f);

        // assert(current_voxel_start.x == current_pos.x || current_voxel_start.y == current_pos.y || current_voxel_start.z == current_pos.z);

        glm::vec3 current_voxel_end =
            current_voxel_start + glm::vec3(size) * sign;

        auto [tmin, tmax] = slab_test(
            current_voxel_start, current_voxel_end, current_pos + bias, dir_inv
        );

        // current_pos = ray.origin + tmax*ray.dir;
        current_pos += (tmax)*ray.dir + bias;

#ifndef NDEBUG
        iterations++;

        if (iterations > 10000) {
            SPDLOG_INFO("Iter count exceeded 1000, {}", invocation_id);
            // return glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        }
#endif
    } while (current_pos.x > 0.0f && current_pos.y > 0.0f &&
             current_pos.z > 0.0f && current_pos.x < 1.0f &&
             current_pos.y < 1.0f && current_pos.z < 1.0f);

    return glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
}

int main(int argc, char** argv) {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%@] %v");
    SPDLOG_INFO("Program Started");

    SPDLOG_INFO("Constructing SvoDag");

    size_t depth = 3;
    SvoDag svodag{depth}; // width = 256;

    long limit = 1<<depth;
    for (long x = 0; x < limit; x++) {
        for (long y = 0; y < limit; y++) {
            for (long z = 0; z < limit; z++) {
                long length = (x - (limit>>1)) * (x - (limit>>1)) + (y - (limit>>1)) * (y - (limit>>1)) +
                              (z - (limit>>1)) * (z - (limit>>1));
                if ((limit>>1)*(limit>>2) < length && length <= (limit>>1)*(limit>>1))
                // if (x == 1) 
                {
                    svodag.insert(
                        x, y, z,
                        glm::vec4(
                            (float)x / (float)limit, (float)y / (float)limit,
                            (float)z / (float)limit, 1.0f
                        )
                    );
                }
            }
        }
    }
    SPDLOG_INFO("Constructed SvoDag");

    SPDLOG_INFO("Raymarching...");
    const int img_width = 1920;
    const int img_height = 1080;

    const float film_width = 1.92f;
    const float film_height = 1.08f;
    const float film_offset = 1.0f; // Corresponds to FOV

    std::vector<std::array<std::byte, 4>> data{};
    data.reserve(img_width * img_height);

    // stb wants first raw, then second row...
    for (int i = 0; i < img_height; i++) {
        for (int j = 0; j < img_width; j++) {
            // SPDLOG_INFO("x: {} y: {}", j, i);

            glm::vec3 origin = glm::vec3(-film_offset, 0.5f, 0.5f);
            glm::vec3 dir = glm::normalize(
                glm::vec3(
                    film_offset,
                    j / static_cast<float>(img_width) * film_width -
                        film_width / 2.0f,
                    i / static_cast<float>(img_height) * film_height -
                        film_height / 2.0f
                )
            );

            // SPDLOG_INFO(std::format("{} {}", origin, dir));

            data.push_back(float_to_255(raymarch(svodag, {origin, dir})));
            // data.push_back(float_to_255(raymarch(
            //     svodag, {{0.0f, j / static_cast<float>(img_width) * film_width,
            //               i / static_cast<float>(img_height) * film_height},
            //              {1.0f, 0.0f, 0.0f}}
            // )));
        }
        // SPDLOG_INFO("{}", i);
    }

    SPDLOG_INFO("Done!");

    stbi_write_png(
        "asdf.png", img_width, img_height, 4, data.data(), img_width * 4
    );

    // SPDLOG_INFO(std::format("{}", raymarch(svodag, {glm::vec3(-1.0f, 0.5f,
    // 0.5f), glm::vec3(1.0f, 0.0f, 0.0f)}).value()));

    return 0;
}
