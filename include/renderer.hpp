#ifndef RENDERER_H
#define RENDERER_H

#include "common.hpp"
#include "vertex.hpp"
#include "ssbo.hpp"
#include "svodag.hpp"

#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

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
    alignas(4) unsigned int max_level;
    alignas(4) unsigned int at_index;
} SvodagMetaData;

class Renderer {
public:
	Renderer(const std::filesystem::path& vs_path, const std::filesystem::path& fs_path);
	Renderer(const Renderer& other) = delete;
	Renderer& operator=(const Renderer& other) = delete;
	Renderer(Renderer&& other) noexcept;
	Renderer& operator=(Renderer&& other) noexcept;

	bool main_loop(const std::function<void ()> f);
    GLFWwindow* get_window() const;

    inline void set_camera_pos(glm::vec3 new_pos) {
        camera_pos = new_pos;
    }

    inline void set_camera_dir(glm::vec3 new_dir) {
        camera_dir = new_dir;
    }

    inline void set_fov(float new_fov) {
        fov = new_fov;
    }

	virtual ~Renderer();
private:
	GLFWwindow* window;
	gl::GLuint vbo;
	gl::GLuint ibo;
	gl::GLuint vao;

    Ssbo<SerializedNode> svodag_ssbo;
    Ssbo<SvodagMetaData> metadata_ssbo;
	gl::GLuint program;

    glm::vec3 camera_pos;
    glm::vec3 camera_dir;
    float fov;
    float aspect;

    bool has_value;
};

#endif
