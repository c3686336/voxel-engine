#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "formatter.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <format>

class Camera {
public:
    Camera(glm::vec3 pos, glm::vec3 dir, float fov, float aspect)
        : pos(pos), dir(dir), half_fov(fov / 2.0f), aspect(aspect) {};
    Camera()
        : pos(glm::vec3(0.0, 0.0, 2.0)), dir(glm::vec3(0.0, 0.0, -1.0)),
          half_fov(glm::pi<float>() / 4.0f), aspect(1.0f) {};

    inline void set_dir(glm::vec3 new_dir) { dir = new_dir; }

    inline void set_pos(glm::vec3 new_pos) { pos = new_pos; }

    inline void set_aspect(float new_aspect) { aspect = new_aspect; };

    inline void move(glm::vec3 dx) { pos += dx; };

    inline glm::vec3 get_dir() { return dir; }

    inline glm::vec3 get_pos() { return pos; }

    inline void set_dir(float pitch, float yaw) {
        // dir = glm::angleAxis(pitch, glm::vec3(1.0, 0.0, 0.0)) *
        //       glm::angleAxis(yaw, glm::vec3(0.0, 1.0, 0.0)) *
        //       glm::vec3(0.0, 0.0, -1.0);
        dir = glm::yawPitchRoll(yaw, pitch, 0.0f) * glm::vec4(0.0, 0.0, -1.0, 0.0);
    }

    inline glm::vec3 camera_x_basis() {
        // Rightwards in view space/NDC, scaled according to fov

        // QND solution; Might need to store the quat instead of dir in the
        // future

        return aspect * tanf(half_fov) * rightward();
    }

    inline glm::vec3 camera_y_basis() {
        return tanf(half_fov) * glm::normalize(glm::cross(glm::cross(dir, up), dir));
    }

    inline void set_fov(float new_fov) { half_fov = new_fov; }

    inline float get_fov() { return half_fov; }

    inline glm::vec3 forward() { return dir; };
    inline glm::vec3 backward() { return -dir; };
    inline glm::vec3 rightward() { return glm::normalize(glm::cross(dir, up)); };
    inline glm::vec3 leftward() { return -rightward(); };
    inline glm::vec3 upward() { return up; };
    inline glm::vec3 downward() { return -up; };

private:
    glm::vec3 pos;
    glm::vec3 dir;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    float half_fov;
    float aspect;
};

#endif // CAMERA_HPP
