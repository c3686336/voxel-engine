#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#include "common.hpp"

#include <glbinding/gl/gl.h>

#include <filesystem>
#include <regex>
#include <type_traits>

static const std::regex include_regex{"#include \"(.+)\""};

template <gl::GLenum type> class Shader {
public:
    Shader() noexcept : shader(0) {}
    Shader(const std::filesystem::path& shader_path) noexcept
        : shader(gl::glCreateShader(type)) {
        std::string source = load_file(shader_path);
        std::stringstream source_stream(source);
        std::string line;
        std::ostringstream preprocessed;
        int i = 2;

        while (std::getline(source_stream, line)) {
            std::smatch match;
            if (std::regex_search(line, match, include_regex)) {
                preprocessed << "#line 1 1" << '\n';
                preprocessed << load_file(std::filesystem::path(match[1]));
                preprocessed << "#line " << i << " 0" << '\n';
            } else if (line ==
                       "#extension GL_ARB_shading_language_include : require") {
                preprocessed << "\n\n";
            } else {
                preprocessed << line << '\n';
            }

            i++;
        }

        source = preprocessed.str();

        const char* source_buf = source.c_str();
        gl::glShaderSource(shader, 1, &source_buf, NULL);
        gl::glCompileShader(shader);

        int buflen;
        gl::glGetShaderiv(shader, gl::GLenum::GL_INFO_LOG_LENGTH, &buflen);
        std::unique_ptr<char[]> buf = std::make_unique<char[]>(buflen);
        gl::glGetShaderInfoLog(shader, buflen, &buflen, buf.get());

        SPDLOG_INFO("Filename: {}", shader_path.string());
        SPDLOG_INFO(buf.get());
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

        int buflen;
        gl::glGetProgramiv(program, gl::GLenum::GL_INFO_LOG_LENGTH, &buflen);
        std::unique_ptr<char[]> buf = std::make_unique<char[]>(buflen);
        gl::glGetProgramInfoLog(program, buflen, &buflen, buf.get());

        SPDLOG_INFO(buf.get());
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
