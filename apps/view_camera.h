#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>


namespace dal {

    class CameraLookAt {

    public:
        glm::vec3 m_pos{ 0 };
        glm::vec3 m_up{ 0, 1, 0 };
        glm::vec3 m_view_direc{ 0, 0, -1 };

    public:
        glm::mat4 make_view_mat() const;

        void move_horizontal(const float x, const float z);

    };

}
