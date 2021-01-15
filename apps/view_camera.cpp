#include "view_camera.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>


namespace dal {

    glm::mat4 CameraLookAt::make_view_mat() const {
        const auto translate = glm::translate(glm::mat4{1}, -this->m_pos);
        const auto rotation_x = glm::rotate(glm::mat4{1}, -this->m_rotations.x, glm::vec3{1, 0, 0});
        const auto rotation_y = glm::rotate(glm::mat4{1}, -this->m_rotations.y, glm::vec3{0, 1, 0});

        return rotation_x * rotation_y * translate;
    }

    void CameraLookAt::move_horizontal(const float x, const float z) {
        const glm::vec4 move_vec{ x, 0, z, 0 };
        const auto rotated = glm::rotate(glm::mat4{1}, this->m_rotations.y, glm::vec3{0, 1, 0}) * move_vec;

        this->m_pos.x += rotated.x;
        this->m_pos.z += rotated.z;
    }

}
