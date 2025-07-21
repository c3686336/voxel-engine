#include "common.hpp"
#include "formatter.hpp"
#include "renderer.hpp"
#include "svodag.hpp"
#include "vertex.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

#include <glm/glm.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

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
        std::filesystem::path("simple.frag")
    );

    bool grabbed = false;
    auto prev_escape_state = GLFW_RELEASE;

    float speed = 3.0f;
    float sensitivity = 4.0f;

    float yaw = 0.0f;
    float pitch = 0.0f;

    bool should_close = false;
    double timer = glfwGetTime();
    while (!should_close) {
        should_close = renderer.main_loop([&](GLFWwindow* window,
                                              Camera& camera) {
            ImGui::SliderFloat("Movement Speed", &speed, 0.0f, 10.0f);
            ImGui::SliderFloat("Mouse Sensitivity", &sensitivity, 0.0f, 4.0f);
            
            double new_time = glfwGetTime();
            float dt = -timer + new_time;
            timer = new_time;

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
                        renderer.get_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED
                    );
                } else {
                    glfwSetInputMode(
                        renderer.get_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL
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
            camera.set_aspect((float)width/(float)height);
        });
    }

    return 0;
}
