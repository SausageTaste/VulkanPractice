#include "view_camera.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>


namespace dal {

    glm::mat4 CameraLookAt::make_view_mat() const {
        return glm::lookAt(this->m_pos, this->m_pos + this->m_view_direc, this->m_up);
    }

    void CameraLookAt::move_horizontal(const float x, const float z) {
        this->m_pos.x += x;
        this->m_pos.z += z;
    }

}
