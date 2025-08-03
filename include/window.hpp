#ifndef WINDOW_HPP
#define WINDOW_HPP

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

class Window {
public:
    Window(int width, int height, const char* name) noexcept;
    ~Window() noexcept;

    Window(Window& other) = delete;
    Window(Window&& other) noexcept;

    Window& operator=(Window& other) = delete;
    Window& operator=(Window&& other) noexcept;

    GLFWwindow* get() const;

private:
    int width;
    int height;

    GLFWwindow* window;
};

#endif
