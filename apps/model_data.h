#pragma once

#include <string>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace dal {


    class Transform {

    public:
        glm::quat m_quat;
        glm::vec3 m_pos;
        float m_scale = 1;

    public:
        glm::mat4 make_mat() const;

    };


    struct PushedConstValues {
        glm::mat4 m_model_mat;
    };

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


    std::vector<RenderUnit> load_dmd_model(const char* const model_name_ext);
    std::vector<RenderUnit> get_test_model();

    RenderUnit get_horizontal_plane(const float x_length, const float z_length);
    RenderUnit get_aabb_box();

}
