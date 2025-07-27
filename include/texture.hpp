#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <glbinding/gl/gl.h>

#include <stb/stb_image.h>

#include <spdlog/spdlog.h>

#include <span>
#include <ranges>

class Texture {
public:
    Texture() noexcept : texture{0} {};
    virtual ~Texture() noexcept;

    Texture(Texture&&) noexcept;
    Texture& operator=(Texture&&) noexcept;

    virtual void parameteri(gl::GLenum property_name, gl::GLint parameter);
    virtual void parameterf(gl::GLenum property_name, gl::GLfloat parameter);
    virtual void bind(gl::GLuint unit) = 0;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

protected:
    gl::GLuint texture;
};

class Texture2D : public Texture {
public:
    template <typename T>
    Texture2D(std::span<T> image, gl::GLsizei levels, gl::GLenum internal_format, gl::GLenum format, gl::GLsizei width, gl::GLsizei height, bool generate_mips) {
        using namespace gl;

        glCreateTextures(GL_TEXTURE_2D, 1, &texture);
        glTextureStorage2D(texture, levels, internal_format, width, height);
        glTextureSubImage2D(texture, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, image);
        if (generate_mips) {
            glGenerateTextureMipmap(texture);
        }
    }

};

class CubeMap : public Texture {
public:
    template <typename T>
    CubeMap(std::array<std::span<T>, 6> images, gl::GLenum internal_format, gl::GLenum format, gl::GLsizei width) {
        using namespace gl;
        
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texture);

        glTextureStorage2D(
            texture,
            1,
            internal_format,
            width,
            width
        );

        glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(texture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        for (auto& [idx, image] : images | std::ranges::views::enumerate) {
            glTextureSubImage3D(
                texture,
                0,
                0,
                0,
                idx,
                width,
                width,
                1,
                format,
                GL_UNSIGNED_BYTE,
                image
            );
        }
    }
};

#endif
