#ifndef RENDERER_H
#define RENDERER_H

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
        metadata_ssbo.register_data(
            {
                glm::inverse(model_mat), model_mat /*Just use 3x3 submatrix*/,
                glm::transpose(glm::inverse(model_mat)), max_level,
                (unsigned int)svodag_ssbo.get_current_index()
            }
        );

        for (auto& elem : model) {
            svodag_ssbo.register_data(elem);
        }

        svodag_ssbo.update_ssbo();
        metadata_ssbo.update_ssbo();

        return metadata_ssbo.get_current_index() - 1;
    }

    inline MatID_t register_material(const Material& material) {
        MatID_t matid = materials.register_material(material).value();
        materials.update_ssbo();

        return matid;
    }

	virtual ~Renderer();
private:
	GLFWwindow* window;
	gl::GLuint vbo;
	gl::GLuint ibo;
	gl::GLuint vao;
	gl::GLuint program;

    SsboList<SerializedNode, 120000> svodag_ssbo;
    SsboList<SvodagMetaData, 1024> metadata_ssbo;
    MaterialList<Material, 1024> materials;

    Camera camera;

    float bias_amt = 0.00044f;
    uint32_t model_select = 0;

    bool has_value;
};

#endif
