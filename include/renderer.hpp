#ifndef RENDERER_H
#define RENDERER_H

#include "buffer.hpp"
#include "common.hpp"
#include "vertex.hpp"
#include "ssbo.hpp"
#include "svodag.hpp"
#include "camera.hpp"
#include "material.hpp"
#include "material_list.hpp"

#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

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
	Renderer(const std::filesystem::path& vs_path, const std::filesystem::path& fs_path);
	Renderer(const Renderer& other) = delete;
	Renderer& operator=(const Renderer& other) = delete;
	Renderer(Renderer&& other) noexcept;
	Renderer& operator=(Renderer&& other) noexcept;

	bool main_loop(const std::function<void (GLFWwindow*, Camera&)> f);
    GLFWwindow* get_window() const;

    inline uint32_t register_model(std::vector<SerializedNode> model, unsigned int max_level, glm::mat4 model_mat /*Model space -> World space*/) {
        metadata_ssbo.data.push_back(
            {
                glm::inverse(model_mat), model_mat /*Just use 3x3 submatrix*/,
                glm::transpose(glm::inverse(model_mat)), max_level,
                (unsigned int)svodag_ssbo.size()
            }
        );

        for (auto& elem : model) {
            svodag_ssbo.push_back(elem);
        }

        svodag_ssbo.upload();

        return metadata_ssbo.data.size() - 1;
    }

    inline MatID_t register_material(const Material& material) {
        MatID_t matid = materials.push_back(material);
        materials.upload();

        return matid;
    }

	virtual ~Renderer();
private:
	GLFWwindow* window;
	gl::GLuint vbo;
	gl::GLuint ibo;
	gl::GLuint vao;
	gl::GLuint program;

    AppendBuffer<SerializedNode, gl::GL_SHADER_STORAGE_BUFFER, 120000> svodag_ssbo;
    VectorBuffer<SvodagMetaData, gl::GL_SHADER_STORAGE_BUFFER, 1024> metadata_ssbo;
    AppendBuffer<Material, gl::GL_SHADER_STORAGE_BUFFER, 1024> materials;

    Camera camera;

    float bias_amt = 0.00044f;
    uint32_t model_select = 0;

    bool has_value;
};

#endif
