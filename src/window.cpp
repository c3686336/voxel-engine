#include "window.hpp"

#include <glbinding/glbinding.h>

#include <utility>

Window::Window(int width, int height, const char* name) noexcept
    : width(width), height(height) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, name, NULL, NULL);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    glbinding::initialize(glfwGetProcAddress);
}

Window::~Window() noexcept {
    if (!window)
        return;
    glfwDestroyWindow(window);
}

Window::Window(Window&& other) noexcept
    : width(other.width), height(other.height), window(other.window) {
    other.window = nullptr;
}

Window& Window::operator=(Window&& other) noexcept {
    using std::swap;

    swap(width, other.width);
    swap(height, other.height);
    swap(window, other.window);

    return *this;
}

GLFWwindow* Window::get() const { return window; }
