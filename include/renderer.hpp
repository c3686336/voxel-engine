#ifndef RENDERER_H
#define RENDERER_H

#include "common.hpp"
#include "vertex.hpp"
#include "ssbo.hpp"
#include "svodag.hpp"

#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

#include <format>
#include <string>
#include <filesystem>
#include <functional>

class Renderer {
public:
	Renderer(const std::filesystem::path& vs_path, const std::filesystem::path& fs_path);
	Renderer(const Renderer& other) = delete;
	Renderer& operator=(const Renderer& other) = delete;
	Renderer(const Renderer&& other) noexcept;
	Renderer& operator=(const Renderer&& other) noexcept;

	bool main_loop(const std::function<void ()> f);

	virtual ~Renderer();
private:
	GLFWwindow* window;
	gl::GLuint vbo;
	gl::GLuint ibo;
	gl::GLuint vao;
    Ssbo<SerializedNode> ssbo;
	gl::GLuint program;
};

#endif
