#ifndef RAII_HPP
#define RAII_HPP

#include <glbinding/glbinding.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <spdlog/spdlog.h>

void ensure_glbinding();
void ensure_imgui(GLFWwindow* window);

class GlbindingRAII {
private:
    inline GlbindingRAII(glbinding::GetProcAddress func) noexcept {
        glbinding::initialize(func);
    }

    GlbindingRAII(GlbindingRAII& other) = delete;
    GlbindingRAII(GlbindingRAII&& other) = delete;

    GlbindingRAII& operator=(GlbindingRAII& other) = delete;
    GlbindingRAII& operator=(GlbindingRAII&& other) = delete;

public:
    friend void ensure_glbinding();
};

class ImGuiRAII {
private:
    inline ImGuiRAII(GLFWwindow* window) noexcept {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init();
    }

    ImGuiRAII(ImGuiRAII& other) = delete;
    ImGuiRAII(ImGuiRAII&& other) = delete;

    ImGuiRAII& operator=(ImGuiRAII& other) = delete;
    ImGuiRAII& operator=(ImGuiRAII&& other) = delete;

public:
    inline ~ImGuiRAII() noexcept {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    friend void ensure_imgui(GLFWwindow* window);
};

#endif
