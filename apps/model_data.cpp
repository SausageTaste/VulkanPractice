#include "model_data.h"

#include <stdexcept>

#include <dal_model_parser.h>
#include <dal_modifier.h>

#include "vert_data.h"
#include "util_windows.h"


namespace dal {

    glm::mat4 Transform::make_mat() const {
        const auto identity = glm::mat4{ 1 };
        const auto scaleMat = glm::scale(identity, glm::vec3{ this->m_scale, this->m_scale , this->m_scale });
        const auto translateMat = glm::translate(identity, this->m_pos);

        return translateMat * glm::mat4_cast(this->m_quat) * scaleMat;
    }

    std::vector<RenderUnit> load_dmd_model(const char* const model_name_ext) {
        std::vector<RenderUnit> result;

        const auto model_path = get_res_path() + "/model/" + model_name_ext;
        const auto file_content = readFile(model_path);
        const auto model_data = parser::parse_model_straight(reinterpret_cast<const uint8_t*>(file_content.data()), file_content.size());
        if (!model_data) {
            throw std::runtime_error{ "failed to parser model file: " + model_path };
        }

        for (const auto& x : model_data->m_render_units) {
            assert(x.m_mesh.m_vertices.size() % 9 == 0);

            const auto indexed_mesh = parser::convert_to_indexed(x.m_mesh);
            auto& output_render_unit = result.emplace_back();

            for (const auto& vert : indexed_mesh.m_vertices) {
                auto& fitted_vert = output_render_unit.m_vertices.emplace_back();
                fitted_vert.pos = vert.m_position;
                fitted_vert.texCoord = vert.m_uv_coords;
                fitted_vert.normal = vert.m_normal;


                fitted_vert.texCoord.y = 1.f - fitted_vert.texCoord.y;
            }

            for (const auto& index : indexed_mesh.m_indices) {
                output_render_unit.m_indices.emplace_back(index);
            }

            {
                output_render_unit.m_material.m_albedo_map = x.m_material.m_albedo_map;
                output_render_unit.m_material.m_roughness = x.m_material.m_roughness;
                output_render_unit.m_material.m_metallic = x.m_material.m_metallic;
            }
        }

        return result;
    }

    std::vector<RenderUnit> get_test_model() {
        return load_dmd_model("yuri_cso2.dmd");
    }

    RenderUnit get_horizontal_plane(const float x_length, const float z_length) {
        RenderUnit result;

        const auto x_length_half = x_length * 0.5f;
        const auto y_length_half = z_length * 0.5f;

        result.m_vertices = {
            {{-x_length_half, 0.f, -z_length}, {0.0f, 1.0f, 0.0f}, { 0,  0}},
            {{-x_length_half, 0.f,  z_length}, {0.0f, 1.0f, 0.0f}, { 0, 10}},
            {{ x_length_half, 0.f,  z_length}, {0.0f, 1.0f, 0.0f}, {10, 10}},
            {{ x_length_half, 0.f, -z_length}, {0.0f, 1.0f, 0.0f}, {10,  0}},
        };

        result.m_indices = {
            0, 1, 2, 0, 2, 3
        };

        result.m_material.m_roughness = 0.7;
        result.m_material.m_metallic = 0;

        return result;
    }

    RenderUnit get_aabb_box() {
        RenderUnit result;

        result.m_vertices = {
            {{-0.5f, 0.f, -0.5f}, glm::normalize(glm::vec3{-1, -1, -1}), {1, 1}}, // 0
            {{-0.5f, 0.f,  0.5f}, glm::normalize(glm::vec3{-1, -1,  1}), {0, 1}}, // 1
            {{ 0.5f, 0.f,  0.5f}, glm::normalize(glm::vec3{ 1, -1,  1}), {1, 1}}, // 2
            {{ 0.5f, 0.f, -0.5f}, glm::normalize(glm::vec3{ 1, -1, -1}), {0, 1}}, // 3
            {{-0.5f, 1.f, -0.5f}, glm::normalize(glm::vec3{-1,  1, -1}), {1, 0}}, // 4
            {{-0.5f, 1.f,  0.5f}, glm::normalize(glm::vec3{-1,  1,  1}), {0, 0}}, // 5
            {{ 0.5f, 1.f,  0.5f}, glm::normalize(glm::vec3{ 1,  1,  1}), {1, 0}}, // 6
            {{ 0.5f, 1.f, -0.5f}, glm::normalize(glm::vec3{ 1,  1, -1}), {0, 0}}, // 7
        };

        result.m_indices = {
            0, 2, 1, 0, 3, 2,
            5, 1, 2, 5, 2, 6,
            6, 2, 3, 6, 3, 7,
            7, 3, 0, 7, 0, 4,
            4, 0, 1, 4, 1, 5,
            4, 5, 6, 4, 6, 7
        };

        result.m_material.m_roughness = 0.1;
        result.m_material.m_metallic = 1;

        return result;
    }

}
