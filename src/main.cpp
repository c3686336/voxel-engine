#include "common.hpp"
#include "components.hpp"
#include "formatter.hpp"
#include "renderable.hpp"
#include "renderer.hpp"
#include "svodag.hpp"
#include "vertex.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <entt/entt.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <filesystem>
#include <format>
#include <string>

using namespace gl;
using namespace glm;

int main(int argc, char** argv) {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%@] %v");

    SPDLOG_INFO("Program Started");

    Renderer renderer(
        std::filesystem::path("simple.vert"),
        std::filesystem::path("simple.frag"), 1920, 1080
    );
    renderer.use_cubemap(
        {"res/right.jpg", "res/left.jpg", "res/top.jpg", "res/bottom.jpg",
         "res/front.jpg", "res/back.jpg"}
    );

    entt::registry registry;

    auto ball1 = registry.create();
    auto ball2 = registry.create();

    SPDLOG_INFO("Creating matid list");
    MatID_t white = renderer.register_material({glm::vec4(1.0, 1.0, 1.0, 1.0)});

    SPDLOG_INFO("Creating SVODAG");

    size_t depth = 6;
    SvoDag svodag{depth}; // width = 256;

    long limit = 1 << depth;
    for (long x = 0; x < limit; x++) {
        for (long y = 0; y < limit; y++) {
            for (long z = 0; z < limit; z++) {
                long length = (x - (limit >> 1)) * (x - (limit >> 1)) +
                              (y - (limit >> 1)) * (y - (limit >> 1)) +
                              (z - (limit >> 1)) * (z - (limit >> 1));
                if ((limit >> 1) * (limit >> 2) < length &&
                    length <= (limit >> 1) * (limit >> 1))
                // if (x == 2)
                {
                    svodag.insert(x, y, z, white);
                }
            }
        }
    }
    SPDLOG_INFO("Created SVODAG");

    // SPDLOG_INFO("Before: {}", svodag.serialize().size());
    svodag.dedup();

    std::vector<SerializedNode> data = svodag.serialize();

    size_t model1 = renderer.register_model(data, svodag.get_level());
    SPDLOG_INFO("After: {}", data.size());

    SPDLOG_INFO("Serialized SVODAG");

    std::vector<entt::entity> balls;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 3; k++) {
                // 1000 balls

                auto ball = registry.create();
                registry.emplace<Renderable>(ball, model1, svodag.get_level());
                registry.emplace<Transformable>(
                    ball,
                    translate(
                        scale(
                            translate(identity<mat4>(), vec3(-0.5f)), vec3(2.0f)
                        ),
                        vec3((float)i, (float)j, (float)k) * 3.f
                    )
                );
            }
        }
    }

    bool grabbed = false;
    auto prev_escape_state = GLFW_RELEASE;

    float speed = 3.0f;
    float sensitivity = 4.0f;

    float yaw = 0.0f;
    float pitch = 0.0f;

    bool should_close = false;
    double timer = glfwGetTime();
    while (!should_close) {
        should_close = renderer.main_loop(
            registry, [&](GLFWwindow* window, Camera& camera) {
                ImGui::SliderFloat("Movement Speed", &speed, 0.0f, 10.0f);
                ImGui::SliderFloat(
                    "Mouse Sensitivity", &sensitivity, 0.0f, 4.0f
                );

                double new_time = glfwGetTime();
                float dt = -timer + new_time;
                timer = new_time;

                ImGui::Text("%f ms, %f FPS", dt * 1000.f, 1.f / dt);

                int width, height;
                glfwGetWindowSize(window, &width, &height);

                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                    camera.move(speed * dt * camera.forward());
                }

                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                    camera.move(speed * dt * camera.leftward());
                }

                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                    camera.move(speed * dt * camera.backward());
                }

                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                    camera.move(speed * dt * camera.rightward());
                }

                if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                    camera.move(speed * dt * camera.upward());
                }

                if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                    camera.move(speed * dt * camera.downward());
                }

                if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS &&
                    prev_escape_state == GLFW_RELEASE) {
                    grabbed = !grabbed;

                    if (grabbed) {
                        glfwSetInputMode(
                            renderer.get_window(), GLFW_CURSOR,
                            GLFW_CURSOR_DISABLED
                        );
                    } else {
                        glfwSetInputMode(
                            renderer.get_window(), GLFW_CURSOR,
                            GLFW_CURSOR_NORMAL
                        );
                    }
                }

                if (grabbed) {
                    double x, y;
                    glfwGetCursorPos(window, &x, &y);

                    float scaled_x = x / (float)width * sensitivity;
                    float scaled_y = y / (float)height * sensitivity;

                    yaw = -scaled_x;
                    pitch = clamp<float>(
                        -scaled_y, -pi<float>() * 0.49, pi<float>() * 0.49
                    );
                }

                prev_escape_state = glfwGetKey(window, GLFW_KEY_ESCAPE);

                camera.set_dir(pitch, yaw);
                camera.set_aspect((float)width / (float)height);
            }
        );
    }

    return 0;
}
