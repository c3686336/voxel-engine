#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include <glm/glm.hpp>

class Transformable {
public:
    Transformable(glm::mat4 matrix) noexcept
        : transform(matrix), inv_transform(glm::inverse(matrix)),
          normal_transform(glm::inverse(glm::transpose(matrix))) {};

    glm::mat4 get_transform() const noexcept { return transform; }
    glm::mat4 get_inv_transform() const noexcept { return inv_transform; }
    glm::mat4 get_normal_transform() const noexcept { return normal_transform; }

    void set_transform(glm::mat4 new_transform) {
        transform = new_transform;
        inv_transform = glm::inverse(new_transform);
        normal_transform = glm::transpose(inv_transform);
    }

private:
    glm::mat4 transform;
    glm::mat4 inv_transform;
    glm::mat4 normal_transform;
};

#endif
