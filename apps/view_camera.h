#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>


namespace dal {

    class CameraLookAt {

    public:
        glm::vec3 m_pos{ 0 };
        glm::vec2 m_rotations{ 0 };

    public:
        glm::mat4 make_view_mat() const;

        void move_horizontal(const float x, const float z);

    };

}
