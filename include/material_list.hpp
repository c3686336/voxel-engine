#ifndef MATERIAL_LIST_HPP
#define MATERIAL_LIST_HPP

#include "ssbo.hpp"

#include <cstdint>
#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <optional>

typedef std::uint32_t MatID_t;

template <class T, size_t L> // L: Size of the buffer, including the `Null`
                             // element in the 0th index. The shader should be
                             // able to distinguish default-constructed
                             // material.
class MaterialList {
public:
    MaterialList() noexcept {
        gl::glGenBuffers(1, &ssbo);
        gl::glBindBuffer(gl::GLenum::GL_SHADER_STORAGE_BUFFER, ssbo);
        glNamedBufferStorage(
            ssbo, L * sizeof(T), materials.data(),
            gl::BufferStorageMask::GL_DYNAMIC_STORAGE_BIT
        );

        has_data = true;
    };

    ~MaterialList() noexcept {
        if (!has_data)
            return;

        gl::glDeleteBuffers(1, &ssbo);
    }

    MaterialList(MaterialList<T, L>& other) noexcept : materials(other.materials), n_materials(other.n_materials), has_data(other.has_data), n_materials_ingpu(other.n_materials) {
        gl::glGenBuffers(1, &ssbo);
        gl::glBindBuffer(gl::GLenum::GL_SHADER_STORAGE_BUFFER, ssbo);
        glNamedBufferStorage(
            ssbo, L * sizeof(T), materials.data(),
            gl::BufferStorageMask::GL_DYNAMIC_STORAGE_BIT
        );
    }

    friend void
    swap(MaterialList<T, L>& first, MaterialList<T, L>& second) noexcept {
        using std::swap;

        swap(first.materials, second.materials);
        swap(first.ssbo, second.ssbo);
        swap(first.n_materials, second.n_materials);
        swap(first.n_materials_ingpu, second.n_materials_ingpu);
        swap(first.has_data, second.has_data);
    }

    MaterialList(MaterialList<T, L>&& other) noexcept
        : materials(other.materials), n_materials(other.n_materials),
          has_data(other.has_data), n_materials_ingpu(n_materials_ingpu),
          ssbo(other.ssbo) {
        other.has_data = false;
    }

    MaterialList<T, L> operator=(MaterialList<T, L>&& other) noexcept {
        std::swap(materials, other.materials);
        std::swap(ssbo, other.ssbo);
        std::swap(n_materials, other.n_materials);
        std::swap(n_materials_ingpu, other.n_materials_ingpu);
        std::swap(has_data, other.has_data);

        return *this;
    }

    MaterialList<T, L> operator=(MaterialList<T, L> other) noexcept {
        using std::swap;
        
        swap(*this, other);

        return *this;
    }

    std::optional<MatID_t> register_material(const T&& material) noexcept {
        if (n_materials >= L)
            return std::nullopt;

        materials[n_materials] = std::move(material);

        n_materials++;

        return std::make_optional(n_materials - 1);
    }

    void update_ssbo() noexcept {
        if (n_materials == n_materials_ingpu)
            return;

        gl::glBufferSubData(
            ssbo, n_materials_ingpu * sizeof(T),
            (n_materials - n_materials_ingpu) * sizeof(T),
            materials.get() + n_materials_ingpu
        );

        n_materials_ingpu = n_materials;
    }

    void bind(unsigned int binding_index) noexcept {
        gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, binding_index, ssbo);
    }

private:
    std::array<T, L> materials;
    size_t n_materials = 1;

    bool has_data;
    size_t n_materials_ingpu = 1;
    gl::GLuint ssbo;
};

#endif
