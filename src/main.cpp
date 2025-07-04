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
        break;
    default:
        SPDLOG_INFO("Non-standard severity marker");
    }
}

int main(int argc, char **argv) {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%@] %v");

    SPDLOG_INFO("Program Started");

    glfwInit();
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                          "voxel engine", NULL, NULL);

    if (window == NULL) {
        SPDLOG_CRITICAL("Could not create the window");
        exit(1);
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);

    glfwShowWindow(window);

    glbinding::initialize(glfwGetProcAddress);

    glEnable(gl::GLenum::GL_DEBUG_OUTPUT);
    glDebugMessageCallback(message_callback, nullptr);

    gl::glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    GLuint vbo;
    glCreateBuffers(1, &vbo);
    glNamedBufferStorage(vbo, sizeof(Vertex)*3, triangle, GL_DYNAMIC_STORAGE_BIT);
    SPDLOG_INFO("Created VBO");

	GLuint ibo;
	glCreateBuffers(1, &ibo);
	glNamedBufferStorage(ibo, sizeof(uint32_t)*3, triangle_indices, GL_DYNAMIC_STORAGE_BIT);
	SPDLOG_INFO("Created IBO");

    GLuint vao;
    glCreateVertexArrays(1, &vao);
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));
    glEnableVertexArrayAttrib(vao, 0);
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos));
	glVertexArrayAttribBinding(vao, 0, 0);

	glVertexArrayElementBuffer(vao, ibo);
    SPDLOG_INFO("created VAO");

	const std::string vs_src = load_file(std::filesystem::path("simple.vert"));
	const char* vs_buf = vs_src.c_str();
	const GLint vs_len = vs_src.length();
	
    const std::string fs_src = load_file(std::filesystem::path("simple.frag"));
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
	SPDLOG_INFO("Created shader program");

    while (!glfwWindowShouldClose(window)) {
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		glUseProgram(program);
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
		
		glfwSwapBuffers(window);
		glfwPollEvents();
    }

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteProgram(program);

	glDeleteShader(vs);
	glDeleteShader(fs);
	
    glfwTerminate();
    return 0;
}
