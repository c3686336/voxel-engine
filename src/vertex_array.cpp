#include "vertex_array.hpp"

using namespace gl;

VertexArray::VertexArray() noexcept : vao(0) {};

VertexArray::VertexArray(
    const Buffer<gl::GL_ARRAY_BUFFER>& vbo,
    const Buffer<gl::GL_ELEMENT_ARRAY_BUFFER>& ibo, gl::GLsizeiptr stride,
    std::initializer_list<std::tuple<
        gl::GLint, gl::GLenum, gl::GLboolean, gl::GLsizeiptr, gl::GLint>>
        attributes
) noexcept {
    glCreateVertexArrays(1, &vao);

    glVertexArrayVertexBuffer(vao, 0, vbo.get(), 0, stride);

    for (auto [i, attrib] : attributes | std::ranges::views::enumerate) {
        auto& [size, type, normalized, offset, binding_index] = attrib;
        glEnableVertexArrayAttrib(vao, i);
        glVertexArrayAttribFormat(vao, i, size, type, normalized, offset);
        glVertexArrayAttribBinding(vao, i, binding_index);
    }

    glVertexArrayElementBuffer(vao, ibo.get());
}

VertexArray::~VertexArray() noexcept { gl::glDeleteVertexArrays(1, &vao); }

VertexArray::VertexArray(VertexArray&& other) noexcept : vao(other.vao) {
    other.vao = 0;
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    using std::swap;

    swap(vao, other.vao);

    return *this;
}

void VertexArray::bind() { gl::glBindVertexArray(vao); }
