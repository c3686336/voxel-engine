#include "program.hpp"
#include "common.hpp"

#include <string>

using namespace std;
using namespace gl;

Program::Program() noexcept : program(0) {}
Program::~Program() noexcept { glDeleteProgram(program); }

Program::Program(Program&& other) noexcept : program(other.program) {
    other.program = 0;
}

Program& Program::operator=(Program&& other) noexcept {
    using std::swap;

    swap(program, other.program);

    return *this;
}

gl::GLuint Program::get() const noexcept { return program; }

void Program::use() const noexcept { glUseProgram(program); }
