#ifndef SSBO_HPP
#define SSBO_HPP

#include <glbinding/gl/gl.h>
#include <vector>

template <typename T> class Ssbo {
public:
    Ssbo(std::vector<T> data) noexcept {
        using namespace gl;

        glGenBuffers(1, &ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glNamedBufferStorage(ssbo, data.size()*sizeof(T), data.data(), GL_DYNAMIC_STORAGE_BIT);
    }

    Ssbo(Ssbo<T>& other) = delete;
    Ssbo(Ssbo<T>&& other) = delete;

    Ssbo<T>& operator=(Ssbo<T>& other) = delete;
    Ssbo<T>& operator=(Ssbo<T>&& other) = delete;

    ~Ssbo() noexcept {
        gl::glDeleteBuffers(1, &ssbo);
    }

    void bind(unsigned int binding_index) {
        gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, binding_index, ssbo);
    }

private:
    gl::GLuint ssbo;
};

#endif
