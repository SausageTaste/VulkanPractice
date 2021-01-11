#pragma once

#include <string>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>


namespace dal {

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texCoord;
    };

    struct Material {
        std::string m_albedo_map;
    };

    struct RenderUnit {
        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;
        Material m_material;
    };


    std::vector<RenderUnit> get_test_model();

}