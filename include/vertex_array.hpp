#ifndef VERTEX_ARRAY_HPP
#define VERTEX_ARRAY_HPP

#include "buffer.hpp"

#include <glbinding/gl/gl.h>

#include <array>
#include <ranges>
#include <tuple>

class VertexArray {
public:
    VertexArray() noexcept;
    VertexArray(
        const Buffer<gl::GL_ARRAY_BUFFER>& vbo,
        const Buffer<gl::GL_ELEMENT_ARRAY_BUFFER>& ibo, gl::GLsizeiptr stride,
        std::initializer_list<std::tuple<
            gl::GLint, gl::GLenum, gl::GLboolean, gl::GLsizeiptr, gl::GLint>>
            attributes
    ) noexcept;

    ~VertexArray() noexcept;

    VertexArray(VertexArray& other) = delete;
    VertexArray(VertexArray&& other) noexcept;

    VertexArray& operator=(VertexArray& other) = delete;
    VertexArray& operator=(VertexArray&& other) noexcept;

    void bind();

private:
    gl::GLuint vao;
};

#endif
