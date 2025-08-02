#ifndef RENDERER_H
#define RENDERER_H

#include "buffer.hpp"
#include "common.hpp"
#include "vertex.hpp"
#include "svodag.hpp"
#include "camera.hpp"
#include "material.hpp"
#include "material_list.hpp"
#include "texture.hpp"

#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

#include <entt/entt.hpp>

#include <format>
#include <string>
#include <filesystem>
#include <functional>

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
	Renderer(const std::filesystem::path& vs_path, const std::filesystem::path& fs_path, int width, int height);
	Renderer(const Renderer& other) = delete;
	Renderer& operator=(const Renderer& other) = delete;
	Renderer(Renderer&& other) noexcept;
	Renderer& operator=(Renderer&& other) noexcept;

	bool main_loop(entt::registry& registry, const std::function<void (GLFWwindow*, Camera&)> f);
    GLFWwindow* get_window() const;

    inline size_t register_model(const std::vector<SerializedNode> model, const unsigned int max_level) {
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
    
	virtual ~Renderer();
private:
    int width;
    int height;
    
	GLFWwindow* window;
	gl::GLuint vbo;
	gl::GLuint ibo;
	gl::GLuint vao;
	gl::GLuint program;

    AppendBuffer<SerializedNode, gl::GL_SHADER_STORAGE_BUFFER> svodag_ssbo;
    VectorBuffer<SvodagMetaData, gl::GL_SHADER_STORAGE_BUFFER> metadata_ssbo;
    AppendBuffer<Material, gl::GL_SHADER_STORAGE_BUFFER> materials;

    std::array<ImmutableBuffer<gl::GL_SHADER_STORAGE_BUFFER>, 2> prev_reservoirs;
    int reservoir_index;
    bool is_first_frame = true;

    Camera camera;
    
    CubeMap cubemap;

    float bias_amt = 0.00044f;
    uint32_t model_select = 0;

    glm::vec4 albedo{0.3f, 0.5f, 0.6f, 1.0f};
    float metallicity = 0.0;
    float roughness = 0.5;

    bool has_value;
};

#endif
