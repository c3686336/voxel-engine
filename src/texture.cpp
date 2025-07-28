#include "texture.hpp"

Texture::~Texture() noexcept {
    gl::glDeleteTextures(1, &texture);
}

Texture::Texture(Texture&& other) noexcept : texture(other.texture) {
    other.texture = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    using std::swap;

    swap(texture, other.texture);

    return *this;
}

void Texture::bind(gl::GLuint unit) {
    gl::glBindTextureUnit(unit, texture);
}

void Texture::parameteri(gl::GLenum property_name, gl::GLint parameter) {
    gl::glTextureParameteri(texture, property_name, parameter);
}

void Texture::parameterf(gl::GLenum property_name, gl::GLfloat parameter) {
    gl::glTextureParameteri(texture, property_name, parameter);
}

Texture2D::Texture2D() noexcept : Texture{} {};
CubeMap::CubeMap() noexcept : Texture{} {};
