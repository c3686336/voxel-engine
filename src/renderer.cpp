#include "renderer.hpp"

#include "common.hpp"
#include "components.hpp"
#include "formatter.hpp"
#include "raii.hpp"
#include "renderable.hpp"
#include "svodag.hpp"
#include "vertex.hpp"

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

Renderer::Renderer(int width, int height)
    : width(width), height(height), window(width, height, "asdf"), vbo(), vao(),
      ibo(), reservoir_index(0), camera(), cubemap(), quad_texture() {
    ensure_glbinding();

    glEnable(GL_FRAMEBUFFER_SRGB);

    vbo = Buffer<gl::GL_ARRAY_BUFFER>(
        sizeof(Vertex) * 3, GL_DYNAMIC_STORAGE_BIT, fullscreen_quad
    );
    ibo = Buffer<gl::GL_ELEMENT_ARRAY_BUFFER>(
        sizeof(uint32_t) * 3, GL_DYNAMIC_STORAGE_BIT, fullscreen_indices
    );
    vao = VertexArray(
        vbo, ibo, sizeof(Vertex),
        {{3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos), 0}}
    );

    SPDLOG_INFO("Creating SSBO");
    svodag_ssbo = AppendBuffer<SerializedNode, GL_SHADER_STORAGE_BUFFER>{10000};
    metadata_ssbo = VectorBuffer<SvodagMetaData, GL_SHADER_STORAGE_BUFFER>{100};
    materials = AppendBuffer<Material, GL_SHADER_STORAGE_BUFFER>{1024};
    materials.push_back(Material{});
    svodag_ssbo.push_back(SerializedNode{});

    prev_reservoirs =
        ImmutableBuffer<GL_SHADER_STORAGE_BUFFER>{width * height * 200};

    quad_texture = Texture2D(1, GL_RGBA32F, GL_RGBA, width, height, false);

    ensure_imgui(window.get());
}

bool Renderer::main_loop(
    entt::registry& registry, const std::function<void(Window&, Camera&)> f
) {
    // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT);

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
                    transformable.get_normal_transform(), renderable.max_level,
                    renderable.model_id
                );
            }
        }
    );

    metadata_ssbo.upload();

    ImGui::SliderFloat("Bias Amount", &bias_amt, 0.0f, .01f, "%.5f");
    ImGui::SliderFloat4("Albedo", glm::value_ptr(albedo), 0.0f, 1.0f);
    ImGui::SliderFloat("Metallicity", &metallicity, 0.0f, 1.0f);
    ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f);
    ImGui::Checkbox("Reuse?", &temporal_reuse);
    ImGui::End();

    restir_first_hit.use();
    bind_everything();
    glDispatchCompute(width / 8, height / 4, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    restir_reuse.use();
    bind_everything();
    glDispatchCompute(width / 8, height / 4, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    quad_renderer.use();
    vao.bind();
    quad_texture.bind(0);

    glDrawElements(gl::GLenum::GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window.get());

    metadata_ssbo.lock();

    is_first_frame = false;
    return glfwWindowShouldClose(window.get());
}

GLFWwindow* Renderer::get_window() const { return window.get(); }

void Renderer::use_cubemap(const std::array<std::filesystem::path, 6>& path) {
    std::array<std::vector<std::byte>, 6> images;
    std::array<std::span<std::byte>, 6> spans;
    int result_width;

    for (int i = 0; i < 6; i++) {
        auto [image, img_width, img_height] = load_image(path[i]);

        images[i] = std::move(image);
        spans[i] = images[i];

        result_width = img_width;
    }

    cubemap = CubeMap(
        spans, gl::GLenum::GL_SRGB8_ALPHA8, gl::GLenum::GL_RGBA, result_width
    );
}

void Renderer::bind_everything() {
    svodag_ssbo.bind(3);
    metadata_ssbo.bind(2);
    materials.bind(6);
    cubemap.bind(0);
    prev_reservoirs.bind(18);
    glUniform1i(12, 0);
    glUniform1i(15, width);
    glUniform1i(16, height);
    glUniform1i(17, is_first_frame);
    glUniform1i(20, temporal_reuse);

    glUniform1f(13, (float)glfwGetTime());

    glm::vec3 x_basis = camera.camera_x_basis();
    glm::vec3 y_basis = camera.camera_y_basis();
    glm::vec3 pos = camera.get_pos();
    glm::vec3 dir = camera.get_dir();

    glUniform3fv(1, 1, glm::value_ptr(pos));
    glUniform3fv(2, 1, glm::value_ptr(dir));

    glUniform3fv(5, 1, glm::value_ptr(x_basis));
    glUniform3fv(4, 1, glm::value_ptr(y_basis));

    glUniform1ui(8, metadata_ssbo.data.size());

    glUniform1f(6, bias_amt);

    glUniform4fv(9, 1, glm::value_ptr(albedo));
    glUniform1f(10, metallicity);
    glUniform1f(11, roughness);

    quad_texture.bind_image(1, 0, GL_WRITE_ONLY, GL_RGBA32F);
}
