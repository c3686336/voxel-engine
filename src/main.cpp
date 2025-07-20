#include "common.hpp"
#include "formatter.hpp"
#include "renderer.hpp"
#include "svodag.hpp"
#include "vertex.hpp"

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

    renderer.set_camera_dir(vec3(0.0, 0.0, -1.0), vec3(0.0, 1.0, 0.0));
    renderer.set_camera_pos(vec3(0.5, 0.5, +2.0));

    vec3 normal_camera_dir = vec3(0.0, 0.0, -1.0);
    vec3 camera_up = vec3(0.0, 1.0, 0.0);

    quat camera_quat = quat(1, 0, 0, 0);
    vec3 camera_pos = vec3(0.5, 0.5, 2.0);

    bool should_close = false;
    double timer = glfwGetTime();
    while (!should_close) {
        should_close = renderer.main_loop([&](GLFWwindow* window) {
            double new_time = glfwGetTime();
            float dt = timer - new_time;
            timer = new_time;

            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
                camera_quat =
                    rotate(camera_quat, -dt * 1.0f, vec3(1.0, 0.0, 0.0));
            }

            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
                camera_quat =
                    rotate(camera_quat, dt * 1.0f, vec3(1.0, 0.0, 0.0));
            }

            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
                camera_quat =
                    rotate(camera_quat, -dt * 1.0f, vec3(0.0, 1.0, 0.0));
            }

            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                camera_quat =
                    rotate(camera_quat, dt * 1.0f, vec3(0.0, 1.0, 0.0));
            }

            vec3 camera_dir = camera_quat * normal_camera_dir;
            vec3 camera_right = cross(camera_dir, camera_up);

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                camera_pos += -camera_dir * dt;
            }

            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                camera_pos += camera_right * dt;
            }

            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                camera_pos += camera_dir * dt;
            }

            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                camera_pos += -camera_right * dt;
            }

            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                camera_pos += -camera_up * dt;
            }

            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                camera_pos += camera_up * dt;
            }

            renderer.set_camera_dir(
                camera_dir, cross(camera_dir, camera_right)
            );
            renderer.set_camera_pos(camera_pos);
        });
    }

    return 0;
}
