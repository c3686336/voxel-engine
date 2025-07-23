#ifndef SSBO_HPP
#define SSBO_HPP

#include <glbinding/gl/gl.h>
#include <vector>
#include <optional>

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

template <class T, size_t L>
class SsboList {
public:
    SsboList() noexcept : has_data(false) {};

    void initialize() noexcept {
        gl::glGenBuffers(1, &ssbo);
        gl::glBindBuffer(gl::GLenum::GL_SHADER_STORAGE_BUFFER, ssbo);
        glNamedBufferStorage(
            ssbo, L * sizeof(T), buffer.data(),
            gl::BufferStorageMask::GL_DYNAMIC_STORAGE_BIT
        );

        has_data = true;
    }

    ~SsboList() noexcept {
        if (!has_data)
            return;

        gl::glDeleteBuffers(1, &ssbo);
    }

    SsboList(SsboList<T, L>& other) noexcept : buffer(other.buffer), n_used(other.n_used), has_data(other.has_data), n_uploaded(other.n_uploaded) {
        gl::glGenBuffers(1, &ssbo);
        gl::glBindBuffer(gl::GLenum::GL_SHADER_STORAGE_BUFFER, ssbo);
        glNamedBufferStorage(
            ssbo, L * sizeof(T), buffer.data(),
            gl::BufferStorageMask::GL_DYNAMIC_STORAGE_BIT
        );
    }

    friend void
    swap(SsboList<T, L>& first, SsboList<T, L>& second) noexcept {
        using std::swap;

        swap(first.buffer, second.buffer);
        swap(first.ssbo, second.ssbo);
        swap(first.n_used, second.n_used);
        swap(first.n_uploaded, second.n_uploaded);
        swap(first.has_data, second.has_data);
    }

    SsboList(SsboList<T, L>&& other) noexcept
        : buffer(other.buffer), n_used(other.n_used),
          has_data(other.has_data), n_uploaded(n_uploaded),
          ssbo(other.ssbo) {
        other.has_data = false;
    }

    SsboList<T, L>& operator=(SsboList<T, L>&& other) noexcept {
        std::swap(buffer, other.buffer);
        std::swap(ssbo, other.ssbo);
        std::swap(n_used, other.n_used);
        std::swap(n_uploaded, other.n_uploaded);
        std::swap(has_data, other.has_data);

        return *this;
    }

    SsboList<T, L>& operator=(SsboList<T, L>& other) noexcept {
        using std::swap;

        swap(*this, SsboList<T, L>(other)); // Copy-and-swap

        return *this;
    }

    std::optional<uint32_t> register_data(const T& data) noexcept {
        if (n_used >= L)
            return std::nullopt;

        buffer[n_used] = data;

        n_used++;

        return std::make_optional(n_used - 1);
    }

    void update_ssbo() noexcept {
        if (n_used == n_uploaded)
            return;

        gl::glNamedBufferSubData(
            ssbo, n_uploaded * sizeof(T),
            (n_used - n_uploaded) * sizeof(T),
            buffer.data() + n_uploaded
        );

        n_uploaded = n_used;
    }

    void bind(unsigned int binding_index) noexcept {
        gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, binding_index, ssbo);
    }

    size_t get_current_index() noexcept {
        return n_used;
    }

private:
    std::array<T, L> buffer;
    size_t n_used = 0;

    bool has_data;
    size_t n_uploaded = 0;
    gl::GLuint ssbo;
};

#endif
