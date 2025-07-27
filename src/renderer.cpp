#include "renderer.hpp"

#include "common.hpp"
#include "formatter.hpp"
#include "svodag.hpp"
#include "vertex.hpp"
#include "renderable.hpp"
#include "components.hpp"

#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <format>
#include <string>

using namespace gl;

void framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
    gl::glViewport(0, 0, width, height);
}

void message_callback(
    GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    GLchar const* message, void const* user_param
) {
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

    std::string messge = std::format(
        "From {} : {}, {}", src_str, type_str, static_cast<int>(id)
    );

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

    GLFWwindow* window =
        glfwCreateWindow(width, height, "voxel engine", NULL, NULL);

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
    glEnable(GL_FRAMEBUFFER_SRGB);
    glDebugMessageCallback(message_callback, nullptr);
    glViewport(0, 0, width, height);
    SPDLOG_INFO("Initialized OpenGL");
}

GLuint create_vbo() {
    GLuint vbo;
    glCreateBuffers(1, &vbo);
    glNamedBufferStorage(
        vbo, sizeof(Vertex) * 3, fullscreen_quad, GL_DYNAMIC_STORAGE_BIT
    );
    SPDLOG_INFO("Created VBO");

    return vbo;
}

GLuint create_ibo() {
    GLuint ibo;
    glCreateBuffers(1, &ibo);
    glNamedBufferStorage(
        ibo, sizeof(uint32_t) * 3, fullscreen_indices, GL_DYNAMIC_STORAGE_BIT
    );
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
    glVertexArrayAttribFormat(
        vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos)
    );
    glVertexArrayAttribBinding(vao, 0, 0);

    glVertexArrayElementBuffer(vao, ibo);

    SPDLOG_INFO("Bound vbo, ibo to the vao");
}

GLuint load_shaders(
    const std::filesystem::path& vs_path, const std::filesystem::path& fs_path
) {
    const std::string vs_src = load_file(vs_path);
    const char* vs_buf = vs_src.c_str();
    const GLint vs_len = vs_src.length();

    const std::string fs_src = load_file(std::filesystem::path(fs_path));
    const GLint fs_len = fs_src.length();
    const char* fs_buf = fs_src.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vs_buf, &vs_len);
    glCompileShader(vs);
    SPDLOG_INFO("Created vertex shader");

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fs_buf, &fs_len);
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

Renderer::Renderer(
    const std::filesystem::path& vs_path, const std::filesystem::path& fs_path
)
    : camera(), has_value(true) {
    window = create_window(640, 480);
    initialize_gl(640, 480);

    vbo = create_vbo();
    ibo = create_ibo();
    vao = create_vao();
    bind_buffers(vao, vbo, ibo);
    program = load_shaders(vs_path, fs_path);

    SPDLOG_INFO("Creating SSBO");
    svodag_ssbo.allocate();
    metadata_ssbo.allocate();
    materials.allocate();
    materials.push_back(Material{});
    svodag_ssbo.push_back(SerializedNode{});

    program = load_shaders(vs_path, fs_path);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
}

Renderer::Renderer(Renderer&& other) noexcept {
    window = other.window;
    vbo = other.vbo;
    ibo = other.vbo;
    vao = other.vao;
    program = other.program;
    metadata_ssbo = std::move(other.metadata_ssbo);
    svodag_ssbo = std::move(other.svodag_ssbo);

    camera = other.camera;

    has_value = other.has_value;
    other.has_value = false;
}

Renderer& Renderer::operator=(Renderer&& other) noexcept {
    using std::swap;

    swap(window, other.window);
    swap(vbo, other.vbo);
    swap(ibo, other.ibo);
    swap(vao, other.vao);
    swap(program, other.program);
    swap(svodag_ssbo, other.svodag_ssbo);
    swap(metadata_ssbo, other.metadata_ssbo);

    camera = other.camera; // Just copy it

    swap(has_value, other.has_value);

    return *this;
}

Renderer::~Renderer() {
    if (!has_value)
        return;

    SPDLOG_INFO("Destroying Renderer");

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(program);

    glfwTerminate();
}

bool Renderer::main_loop(entt::registry& registry, const std::function<void(GLFWwindow*, Camera&)> f) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("asdf");

    f(window, camera);

    metadata_ssbo.data.clear();
    registry.view<Renderable, Transformable>().each(
        [&](auto entity, Renderable& renderable, Transformable& transformable) {
            if (renderable.visible) {
                metadata_ssbo.data.emplace_back(
                    transformable.get_inv_transform(),
                    transformable.get_transform(),
                    transformable.get_normal_transform(),
                    renderable.max_level,
                    renderable.model_id
                );
            }
        }
    );
    
    metadata_ssbo.upload();

    glUseProgram(program);
    glBindVertexArray(vao);
    svodag_ssbo.bind(3);
    metadata_ssbo.bind(2);
    materials.bind(6);

    glm::vec3 x_basis = camera.camera_x_basis();
    glm::vec3 y_basis = camera.camera_y_basis();
    glm::vec3 pos = camera.get_pos();
    glm::vec3 dir = camera.get_dir();

    glUniform3fv(1, 1, glm::value_ptr(pos));
    glUniform3fv(2, 1, glm::value_ptr(dir));

    glUniform3fv(5, 1, glm::value_ptr(x_basis));
    glUniform3fv(4, 1, glm::value_ptr(y_basis));

    glUniform1ui(8, metadata_ssbo.data.size());

    ImGui::SliderFloat("Bias Amount", &bias_amt, 0.0f, .01f, "%.5f");
    glUniform1f(6, bias_amt);

    ImGui::End();

    glDrawElements(gl::GLenum::GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);

    metadata_ssbo.lock();

    return glfwWindowShouldClose(window);
}

GLFWwindow* Renderer::get_window() const { return window; }
