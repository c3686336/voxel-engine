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
    Buffer() noexcept : buffer(0) {};

    Buffer(
        size_t size, gl::BufferStorageMask flags, const void* data = nullptr
    ) noexcept {
        using namespace gl;
        glGenBuffers(1, &buffer);
        glBindBuffer(type, buffer);
        glNamedBufferStorage(buffer, size, data, flags);
    }

    virtual ~Buffer() noexcept { gl::glDeleteBuffers(1, &buffer); }

    Buffer(Buffer& other) = delete;
    Buffer(Buffer&& other) noexcept : buffer(other.buffer) { other.buffer = 0; }

    Buffer& operator=(Buffer& other) = delete;
    Buffer& operator=(Buffer&& other) noexcept {
        using std::swap;

        swap(buffer, other.buffer);

        return *this;
    }

    virtual void bind(gl::GLuint binding_index) const {
        gl::glBindBufferBase(type, binding_index, buffer);
    }

    bool owns() const { return buffer != 0; }

    gl::GLuint get() const { return buffer; }

private:
    gl::GLuint buffer;
};

template <gl::GLenum type> class ImmutableBuffer : public Buffer<type> {
public:
    ImmutableBuffer() noexcept : Buffer<type>() {};

    ImmutableBuffer(gl::GLsizeiptr size)
        : Buffer<type>(size, (gl::BufferStorageMask)0) {};

private:
};

template <gl::GLenum type> class MutableBuffer : public Buffer<type> {
    // Abstracts a mutable buffer object by the use of persistent mapping. It
    // does not own the data.
public:
    MutableBuffer() noexcept
        : Buffer<type>(), size(), ptr(), sync(), index(0), n(0),
          allocation_size(0) {};
    virtual ~MutableBuffer() noexcept {
        if (this->owns()) {
            for (int i = 0; i < 3; i++) {
                gl::glDeleteSync(sync[i]);
            }
        }
    }

    MutableBuffer(MutableBuffer& other) =
        delete; // Copy must be implemented by the wrapping class
    MutableBuffer(MutableBuffer&& other) noexcept = default;

    MutableBuffer& operator=(MutableBuffer& other) = delete; // Same here
    MutableBuffer& operator=(MutableBuffer&& other) noexcept = default;

    MutableBuffer(size_t size, int n)
        : Buffer<type>(
              size * n, gl::GL_MAP_WRITE_BIT | gl::GL_MAP_PERSISTENT_BIT |
                            gl::GL_MAP_COHERENT_BIT
          ),
          size(std::vector<gl::GLsizeiptr>(n)), ptr(),
          sync(
              std::vector<gl::GLsync>(
                  n, gl::glFenceSync(gl::GL_SYNC_GPU_COMMANDS_COMPLETE, 0)
              )
          ),
          index(0), n(n), allocation_size(size) {
        using namespace gl;

        unsigned char* pointer = (unsigned char*)glMapNamedBufferRange(
            this->get(), 0, n * allocation_size,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT
        );

        for (int i = 0; i < n; i++) {
            ptr.push_back(pointer + size * i);
        }
    }

    bool is_available_for_write(gl::GLuint64 timeout_ns = 0) {
        gl::GLenum result = gl::glClientWaitSync(
            sync[index], gl::GL_SYNC_FLUSH_COMMANDS_BIT, timeout_ns
        );

        return result == gl::GLenum::GL_ALREADY_SIGNALED ||
               result == gl::GLenum::GL_CONDITION_SATISFIED;
    }

    virtual void bind(gl::GLuint binding_index) const override {
        gl::glBindBufferRange(
            type, binding_index, this->get(), allocation_size * index,
            allocation_size
        );
    }

    // Also intended to be used at most once per frame.
    void lock() {
        sync[index] =
            gl::glFenceSync(gl::GLenum::GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        index = (index + 1) % n;
    }

protected:
    template <typename T> void write_raw(std::span<T> data) {
        if (!this->owns()) {
            SPDLOG_INFO("Attempting to write before allocating");
            return;
        }

        if (data.size_bytes() > allocation_size) {
            SPDLOG_CRITICAL(
                "glBufferSubdata anticipated to be out of range, {} > {}",
                data.size_bytes(), allocation_size
            );
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
    std::vector<gl::GLsizeiptr> size;
    std::vector<unsigned char*> ptr;
    std::vector<gl::GLsync> sync;

    int index;
    int n;
    size_t allocation_size;
};

template <typename T, gl::GLenum type>
class AppendBuffer : public Buffer<type> {
    // A buffer that can only be extended
public:
    AppendBuffer() noexcept
        : cpu_buffer(), used(0), Buffer<type>(), max_len(0) {};
    AppendBuffer(AppendBuffer<T, type>&& other) noexcept = default;
    AppendBuffer(size_t length) noexcept
        : Buffer<type>(length * sizeof(T), gl::GL_DYNAMIC_STORAGE_BIT), used(0),
          max_len(length) {
        cpu_buffer.reserve(length);
    };

    AppendBuffer& operator=(AppendBuffer<T, type>&& other) noexcept = default;

    size_t push_back(const T& obj) {
        if (cpu_buffer.size() == max_len) {
            throw std::range_error("The buffer is full");
        }

        cpu_buffer.push_back(obj);

        return cpu_buffer.size() - 1;
    }

    size_t size() { return cpu_buffer.size(); }

    void upload() {
        gl::glNamedBufferSubData(
            this->get(), 0, cpu_buffer.size() * sizeof(T), cpu_buffer.data()
        );

        used = cpu_buffer.size() * sizeof(T);
    }

private:
    std::vector<T> cpu_buffer;
    gl::GLintptr used;
    size_t max_len;
};

template <typename T, gl::GLenum type>
class VectorBuffer : public MutableBuffer<type> {
public:
    VectorBuffer() noexcept : data() {};
    VectorBuffer(size_t length) noexcept
        : data(), MutableBuffer<type>(length * sizeof(T), 3) {
        data.reserve(length);
    }
    VectorBuffer(VectorBuffer<T, type>&& other) noexcept = default;

    VectorBuffer& operator=(VectorBuffer<T, type>&& other) noexcept = default;

    void upload() {
        // Blocks
        this->write_raw(std::span(data));
    }

    std::vector<T> data;

private:
};

#endif
