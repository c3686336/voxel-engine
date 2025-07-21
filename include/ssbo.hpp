#ifndef SSBO_HPP
#define SSBO_HPP

#include <glbinding/gl/gl.h>
#include <vector>

template <typename T> class Ssbo {
public:
    Ssbo() noexcept : has_buffer(false) {};

    Ssbo(std::vector<T> data) noexcept {
        using namespace gl;

        glGenBuffers(1, &ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glNamedBufferStorage(
            ssbo, data.size() * sizeof(T), data.data(), GL_DYNAMIC_STORAGE_BIT
        );

        has_buffer = true;
    }

    Ssbo(Ssbo<T>& other) = delete;
    Ssbo(Ssbo<T>&& other) = delete;

    Ssbo<T>& operator=(Ssbo<T>& other) = delete;
    Ssbo<T>& operator=(Ssbo<T>&& other) noexcept {
        using std::swap;

        swap(ssbo, other.ssbo);
        swap(has_buffer, other.has_buffer);

        return *this;
    }

    ~Ssbo() noexcept { if (has_buffer) gl::glDeleteBuffers(1, &ssbo); }

    void bind(unsigned int binding_index) {
        if (has_buffer)
            gl::glBindBufferBase(
                gl::GL_SHADER_STORAGE_BUFFER, binding_index, ssbo
            );
    }

    friend void swap(Ssbo<T>& a, Ssbo<T>& b) {
        using std::swap;
        swap(a.ssbo, b.ssbo);
        swap(a.has_buffer, b.has_buffer);
    }

private:
    gl::GLuint ssbo;
    bool has_buffer;
};

#endif
