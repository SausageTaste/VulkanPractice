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

    std::vector<RenderUnit> get_test_model() {
        std::vector<RenderUnit> result;

        const auto model_path = get_res_path() + "/model/yuri_cso2.dmd";
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
            }
        }

        return result;
    }

}
