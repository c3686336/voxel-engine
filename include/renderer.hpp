#ifndef RENDERER_H
#define RENDERER_H

#include "buffer.hpp"
#include "camera.hpp"
#include "common.hpp"
#include "material.hpp"
#include "material_list.hpp"
#include "program.hpp"
#include "raii.hpp"
#include "svodag.hpp"
#include "texture.hpp"
#include "vertex.hpp"
#include "vertex_array.hpp"
#include "window.hpp"

#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

#include <entt/entt.hpp>

#include <filesystem>
#include <format>
#include <functional>
#include <string>

typedef struct alignas(16) {
    alignas(16) glm::mat4 model_inv;
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 model_norm;
    alignas(4) unsigned int max_level;
    alignas(4) unsigned int at_index;
} SvodagMetaData;

typedef SimpleMaterial Material;

class Renderer {
public:
    Renderer(int width, int height);

    bool main_loop(
        entt::registry& registry, const std::function<void(Window&, Camera&)> f
    );
    GLFWwindow* get_window() const;

    inline size_t register_model(
        const std::vector<SerializedNode> model, const unsigned int max_level
    ) {
        size_t id = svodag_ssbo.size();

        for (auto& elem : model) {
            svodag_ssbo.push_back(elem);
        }

        svodag_ssbo.upload();

        return id;
    }

    inline MatID_t register_material(const Material& material) {
        MatID_t matid = materials.push_back(material);
        materials.upload();

        return matid;
    }

    void use_cubemap(const std::array<std::filesystem::path, 6>&);

private:
    void bind_everything();

    int width;
    int height;

    Window window;
    Buffer<gl::GL_ARRAY_BUFFER> vbo;
    VertexArray vao;
    Buffer<gl::GL_ELEMENT_ARRAY_BUFFER> ibo;

    Program quad_renderer = Program{
        Shader<gl::GL_VERTEX_SHADER>(std::filesystem::path("simple.vert")),
        Shader<gl::GL_FRAGMENT_SHADER>(
            std::filesystem::path("draw_texture.frag")
        )
    };

    Program micro_restir_first_hit = Program{
        Shader<gl::GL_COMPUTE_SHADER>(std::filesystem::path("0_first_hit.comp"))
    };

    Program micro_restir_sample_generation =
        Program{Shader<gl::GL_COMPUTE_SHADER>(
            std::filesystem::path("1_sample_generation.comp")
        )};

    Program micro_restir_temporal_reuse = Program{Shader<gl::GL_COMPUTE_SHADER>(
        std::filesystem::path("2_temporal_reuse.comp")
    )};

    Program micro_restir_spatial_reuse = Program{Shader<gl::GL_COMPUTE_SHADER>(
        std::filesystem::path("3_spatial_reuse.comp")
    )};

    Program micro_restir_shade = Program{
        Shader<gl::GL_COMPUTE_SHADER>(std::filesystem::path("4_shade.comp"))
    };

    AppendBuffer<SerializedNode, gl::GL_SHADER_STORAGE_BUFFER> svodag_ssbo;
    VectorBuffer<SvodagMetaData, gl::GL_SHADER_STORAGE_BUFFER> metadata_ssbo;
    AppendBuffer<Material, gl::GL_SHADER_STORAGE_BUFFER> materials;

    ImmutableBuffer<gl::GL_SHADER_STORAGE_BUFFER> reservoirs;
    ImmutableBuffer<gl::GL_SHADER_STORAGE_BUFFER> prev_reservoirs;
    bool is_first_frame = true;

    Camera camera;

    CubeMap cubemap;
    Texture2D quad_texture;

    float bias_amt = 0.00044f;
    float surface_bias_amt = 0.00187f;
    uint32_t model_select = 0;

    glm::vec4 albedo{0.3f, 0.5f, 0.6f, 1.0f};
    float metallicity = 0.0;
    float roughness = 0.5;

    bool temporal_reuse = true;
    bool spatial_reuse = true;
    bool spatial_first = true;

    bool debug_normal_view = false;
    bool debug_pos_view = false;
    bool debug_weight_view = false;
    bool debug_ignore_shadow = false;

    int initial_sample_count = 8;
};

#endif
