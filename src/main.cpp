#include "common.hpp"
#include "formatter.hpp"
#include "renderer.hpp"
#include "svodag.hpp"
#include "vertex.hpp"

#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <format>
#include <string>

using namespace gl;

int main(int argc, char** argv) {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%@] %v");

    SPDLOG_INFO("Program Started");

    Renderer renderer(
        std::filesystem::path("simple.vert"),
        std::filesystem::path("simple.frag")
    );



    bool should_close = false;
    while (!should_close) {
        should_close = renderer.main_loop([&](GLFWwindow* window,
                                              Camera& camera) {
            double new_time = glfwGetTime();
    }

    return 0;
}
