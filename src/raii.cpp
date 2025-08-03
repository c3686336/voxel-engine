#include "raii.hpp"

void ensure_glbinding() { static GlbindingRAII raii{glfwGetProcAddress}; }

void ensure_imgui(GLFWwindow* window) { static ImGuiRAII raii{window}; }
