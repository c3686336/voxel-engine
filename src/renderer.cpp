#include "renderer.hpp"

#include "common.hpp"
#include "vertex.hpp"

#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

#include <format>
#include <string>
#include <filesystem>

using namespace gl;

void framebuffer_resize_callback(GLFWwindow *window, int width, int height) {
    gl::glViewport(0, 0, width, height);
}

void message_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                      GLsizei length, GLchar const *message,
                      void const *user_param) {
    // Adapted from
    // https://github.com/fendevel/Guide-to-Modern-OpenGL-Functions?tab=readme-ov-file#detailed-messages-with-debug-output

    auto const src_str = [source]() {
        switch (source) {
		case GL_DEBUG_SOURCE_API:
			return "API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            return "WINDOW SYSTEM";
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            return "SHADER COMPILER";
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            return "THIRD PARTY";
        case GL_DEBUG_SOURCE_APPLICATION:
            return "APPLICATION";
        case GL_DEBUG_SOURCE_OTHER:
            return "OTHER";
        default:
            return "";
        }
    }();

    auto const type_str = [type]() {
        switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            return "ERROR";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            return "DEPRECATED_BEHAVIOR";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            return "UNDEFINED_BEHAVIOR";
        case GL_DEBUG_TYPE_PORTABILITY:
            return "PORTABILITY";
        case GL_DEBUG_TYPE_PERFORMANCE:
            return "PERFORMANCE";
        case GL_DEBUG_TYPE_MARKER:
            return "MARKER";
        case GL_DEBUG_TYPE_OTHER:
            return "OTHER";
        default:
            return "";
        }
    }();

    std::string messge = std::format("From {} : {}, {}", src_str, type_str,
                                     static_cast<int>(id));

    switch (severity) {
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        SPDLOG_INFO(message);
        break;
	case GL_DEBUG_SEVERITY_LOW:
        SPDLOG_WARN(message);
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        SPDLOG_ERROR(message);
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        SPDLOG_CRITICAL(message);
		exit(1);
        break;
    default:
        SPDLOG_CRITICAL("Non-standard severity marker");
		exit(1);
    }
}

GLFWwindow* create_window(int width, int height) {
	glfwInit();
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(width, height,
		"voxel engine", NULL, NULL);

	if (window == nullptr) {
		SPDLOG_CRITICAL("Could not create the window");
		exit(1);
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);

	glfwShowWindow(window);

	SPDLOG_INFO("Created the window");

	return window;
}

void initialize_gl(int width, int height) {
	glbinding::initialize(glfwGetProcAddress);
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(message_callback, nullptr);
	glViewport(0, 0, width, height);
	SPDLOG_INFO("Initialized OpenGL");
}

GLuint create_vbo() {
	GLuint vbo;
    glCreateBuffers(1, &vbo);
    glNamedBufferStorage(vbo, sizeof(Vertex)*3, fullscreen_quad, GL_DYNAMIC_STORAGE_BIT);
    SPDLOG_INFO("Created VBO");

	return vbo;
}

GLuint create_ibo() {
	GLuint ibo;
	glCreateBuffers(1, &ibo);
	glNamedBufferStorage(ibo, sizeof(uint32_t)*3, fullscreen_indices, GL_DYNAMIC_STORAGE_BIT);
	SPDLOG_INFO("Created IBO");

	return ibo;
}

GLuint create_vao() {
	GLuint vao;
    glCreateVertexArrays(1, &vao);
    
    SPDLOG_INFO("created VAO");

	return vao;
}

void bind_buffers(GLuint vao, GLuint vbo, GLuint ibo) {
	glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));
    glEnableVertexArrayAttrib(vao, 0);
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos));
	glVertexArrayAttribBinding(vao, 0, 0);

	glVertexArrayElementBuffer(vao, ibo);

	SPDLOG_INFO("Bound vbo, ibo to the vao");
}

GLuint load_shaders(const std::filesystem::path& vs_path, const std::filesystem::path& fs_path) {
	const std::string vs_src = load_file(vs_path);
	const char* vs_buf = vs_src.c_str();
	const GLint vs_len = vs_src.length();
	
    const std::string fs_src = load_file(std::filesystem::path(fs_path));
	const GLint fs_len = fs_src.length();
	const char* fs_buf = fs_src.c_str();

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(
		vs,
		1,
		&vs_buf,
		&vs_len
	);
	glCompileShader(vs);
	SPDLOG_INFO("Created vertex shader");

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(
		fs,
		1,
		&fs_buf,
		&fs_len
	);
	glCompileShader(fs);
	SPDLOG_INFO("Created fragment shader");

	GLuint program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);

	glDeleteShader(vs);
	glDeleteShader(fs);
	
	SPDLOG_INFO("Created shader program");

	return program;
}

Renderer::Renderer(const std::filesystem::path& vs_path, const std::filesystem::path& fs_path) {
	window = create_window(640, 480);
	initialize_gl(640, 480);

	vbo = create_vbo();
	ibo = create_ibo();
	vao = create_vao();
	bind_buffers(vao, vbo, ibo);

    // TODO: Separate this out
    SPDLOG_INFO("Creating SSBO");
    std::array<int, 1> data = {0};
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glNamedBufferStorage(ssbo, sizeof(data), data.data(), GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);
    SPDLOG_INFO("Created SSBO");
	
	program = load_shaders(vs_path, fs_path);
}

Renderer::Renderer(const Renderer&& other) noexcept {
	window = other.window;
	vbo = other.vbo;
	ibo = other.vbo;
	vao = other.vao;
	program = other.program;
}

Renderer& Renderer::operator=(const Renderer&& other) noexcept {
	window = other.window;
	vbo = other.vbo;
	ibo = other.vbo;
	vao = other.vao;
	program = other.program;

	return *this;
}

Renderer::~Renderer() {
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteProgram(program);

    glfwTerminate();
}

bool Renderer::main_loop(const std::function<void ()> f) {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(program);
	glBindVertexArray(vao);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);
	glDrawElements(gl::GLenum::GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

	glfwSwapBuffers(window);
	glfwPollEvents();

	f();
	
	return glfwWindowShouldClose(window);
}
