#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#include "common.hpp"

#include <glbinding/gl/gl.h>

#include <filesystem>
#include <type_traits>

template <gl::GLenum type> class Shader {
public:
    Shader() noexcept : shader(0) {}
    Shader(const std::filesystem::path& shader_path) noexcept
        : shader(gl::glCreateShader(type)) {
        std::string source = load_file(shader_path);
        const char* source_buf = source.c_str();
        gl::glShaderSource(shader, 1, &source_buf, NULL);
        gl::glCompileShader(shader);
    }
    ~Shader() noexcept { gl::glDeleteShader(shader); }

    Shader(Shader& other) = delete;
    Shader(Shader&& other) noexcept : shader(other.shader) { other.shader = 0; }

    Shader& operator=(Shader& other) = delete;
    Shader& operator=(Shader&& other) noexcept {
        using std::swap;

        swap(shader, other.shader);
    }

    gl::GLuint get() const noexcept { return shader; }

private:
    gl::GLuint shader;
};

template <typename T> struct is_shader : std::false_type {};

template <gl::GLenum type> struct is_shader<Shader<type>> : std::true_type {};

template <typename T>
concept IsShader = is_shader<T>::value;

class Program {
public:
    Program() noexcept;

    // TODO: Validate shader combination
    template <typename... Args>
    Program(Args&&... args) : program(gl::glCreateProgram()) {
        attach(std::forward<Args>(args)...);

        gl::glLinkProgram(program);
    }

    ~Program() noexcept;

    Program(Program& other) = delete;
    Program(Program&& other) noexcept;

    Program& operator=(Program& other) = delete;
    Program& operator=(Program&& other) noexcept;

    gl::GLuint get() const noexcept;

    void use() const noexcept;

private:
    template <class U, class... Args>
        requires IsShader<U>
    void attach(U&& arg, Args&&... args) noexcept {
        attach<U>(std::forward<U>(arg));
        attach<Args...>(std::forward<Args>(args)...);
    }

    template <class U>
        requires IsShader<U>
    void attach(U&& arg) noexcept {
        gl::glAttachShader(program, arg.get());
    }

    gl::GLuint program;
};

#endif
