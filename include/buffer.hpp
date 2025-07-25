#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <algorithm>
#include <concepts>
#include <cstring>
#include <glbinding/gl/gl.h>
#include <optional>
#include <span>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <vector>

template <gl::GLenum type> class Buffer {
    // A simple, owning buffer
public:
    Buffer() noexcept : owns_buffer(false) {};
    virtual ~Buffer() noexcept {
        if (owns_buffer) {
            gl::glDeleteBuffers(1, &buffer);
        }
    }

    Buffer(Buffer& other) = delete;
    Buffer(Buffer&& other) noexcept
        : buffer(other.buffer), owns_buffer(other.owns_buffer) {
        other.owns_buffer = false;
    }

    Buffer& operator=(Buffer& other) = delete;
    Buffer& operator=(Buffer&& other) noexcept {
        using std::swap;

        swap(buffer, other.buffer);
        swap(owns_buffer, other.owns_buffer);

        return *this;
    }

    virtual void allocate() = 0;

    virtual void bind(gl::GLuint binding_index) {
        if (owns_buffer) {
            gl::glBindBufferBase(type, binding_index, buffer);
        }
    }

    bool owns() { return owns_buffer; }

protected:
    gl::GLuint buffer;
    bool owns_buffer;
};

template <typename T, gl::GLenum type, size_t max_len, int N = 3>
class MutableBuffer : public Buffer<type> {
    // Abstracts a mutable buffer object by the use of persistent mapping. It
    // does not own the data.
public:
    MutableBuffer() noexcept : Buffer<type>(), index(0) {};
    virtual ~MutableBuffer() noexcept {
        if (this->owns_buffer) {
            for (int i = 0; i < 3; i++) {
                gl::glDeleteSync(sync[i]);
            }
        }
    }

    MutableBuffer(MutableBuffer& other) =
        delete; // Copy must be implemented by the wrapping class
    MutableBuffer(MutableBuffer&& other) noexcept
        : Buffer<type>(std::move(other)) {
        index = other.index;
    }

    MutableBuffer& operator=(MutableBuffer& other) = delete; // Same here
    MutableBuffer& operator=(MutableBuffer&& other) noexcept {
        using std::swap;

        Buffer<type>::operator=(std::move(other));

        return *this;
    }

    void allocate() {
        using namespace gl;

        if (this->owns_buffer) {
            gl::glDeleteBuffers(1, &this->buffer);
        }

        glGenBuffers(1, &this->buffer);
        glBindBuffer(type, this->buffer);
        glNamedBufferStorage(
            this->buffer, allocation_size * N, nullptr,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT
        );

        // ptr[0] = (T*)glMapNamedBuffer(this->buffer, GL_WRITE_ONLY);
        ptr[0] = (T*)glMapNamedBufferRange(this->buffer, 0, N * allocation_size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

        for (int i = 1; i < N; i++) {
            ptr[i] = ptr[0] + max_len * i;
        }

        for (int i=0;i<N;i++) {
            sync[i] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        }

        this->owns_buffer = true;
    }

    bool is_available_for_write(gl::GLuint64 timeout_ns = 0) {
        gl::GLenum result = gl::glClientWaitSync(
            sync[index], gl::GL_SYNC_FLUSH_COMMANDS_BIT, timeout_ns
        );

        return result == gl::GLenum::GL_ALREADY_SIGNALED ||
               result == gl::GLenum::GL_CONDITION_SATISFIED;
    }

    void bind(gl::GLuint binding_index) {
        if (this->owns_buffer) {
            gl::glBindBufferRange(
                type, binding_index, this->buffer, allocation_size * index,
                allocation_size
            );
        }
    }

    // Also intended to be used at most once per frame.
    void lock() {
        sync[index] =
            gl::glFenceSync(gl::GLenum::GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        index = (index + 1) % N;
    }

protected:
    void write_raw(std::span<T> data) {
        if (!this->owns_buffer) {
            SPDLOG_INFO("Attempting to write before allocating");
            return;
        }

        if (data.size_bytes() > allocation_size) {
            SPDLOG_CRITICAL("glBufferSubdata anticipated to be out of range");
            throw std::range_error(
                "glBufferSubdata anticipated to be out of range"
            );
        }

        while (!is_available_for_write(10)) {
        };

        size[index] = data.size_bytes();
        std::memcpy(ptr[index], data.data(), allocation_size);
    }

private:
    std::array<gl::GLsizeiptr, N> size;
    std::array<T*, N> ptr;
    std::array<gl::GLsync, N> sync;

    int index;
    const size_t allocation_size = sizeof(T) * max_len;
};

template <typename T, gl::GLenum type, size_t max_len>
class AppendBuffer : public Buffer<type> {
    // A buffer that can only be extended
public:
    AppendBuffer() : cpu_buffer(), used(0), Buffer<type>() {
        cpu_buffer.reserve(max_len);
    };

    void allocate() {
        gl::glGenBuffers(1, &this->buffer);
        gl::glBindBuffer(gl::GL_SHADER_STORAGE_BUFFER, this->buffer);
        gl::glNamedBufferStorage(
            this->buffer, max_len * sizeof(T), nullptr,
            gl::GL_DYNAMIC_STORAGE_BIT
        );

        this->owns_buffer = true;
    }

    size_t push_back(const T& obj) {
        if (cpu_buffer.size() == max_len) {
            throw std::range_error("The buffer is full");
        }

        cpu_buffer.push_back(obj);

        return cpu_buffer.size() - 1;
    }

    size_t size() { return cpu_buffer.size(); }

    void upload() {
        // gl::glNamedBufferSubData(
        //     this->buffer, used, cpu_buffer.size()*sizeof(T) - used,
        //     (char*)cpu_buffer.data() + used
        // );
        gl::glNamedBufferSubData(this->buffer, 0, cpu_buffer.size() * sizeof(T), cpu_buffer.data());

        used = cpu_buffer.size()*sizeof(T);
    }

private:
    std::vector<T> cpu_buffer;
    gl::GLintptr used;
};

template <typename T, gl::GLenum type, size_t max_len, int N = 3>
class VectorBuffer : public MutableBuffer<T, type, max_len, N> {
public:
    VectorBuffer() noexcept : data() { data.reserve(max_len); }

    void upload() {
        // Blocks
        this->write_raw(std::span(data));
    }

    std::vector<T> data;

private:
};

#endif
