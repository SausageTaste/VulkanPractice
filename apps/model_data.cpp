#include "model_data.h"

#include <dal_model_parser.h>
#include <dal_modifier.h>

#include "vert_data.h"
#include "util_windows.h"


namespace dal {

    ModelData get_test_model() {
        ModelData result;

        const auto model_path = get_res_path() + "/model/irin.dmd";
        const auto file_content = readFile(model_path);
        const auto model_data = parser::parse_model_straight(reinterpret_cast<const uint8_t*>(file_content.data()), file_content.size());
        if (!model_data) {
            throw std::runtime_error{ "failed to parser model file: irin.dmd" };
        }

        for (const auto& x : model_data->m_render_units) {
            assert(x.m_mesh.m_vertices.size() % 9 == 0);

            const auto indexed_mesh = parser::convert_to_indexed(x.m_mesh);

            for (const auto& vert : indexed_mesh.m_vertices) {
                auto& fitted_vert = result.m_vertices.emplace_back();
                fitted_vert.pos = vert.m_position;
                fitted_vert.texCoord = vert.m_uv_coords;
                fitted_vert.normal = vert.m_normal;
            }

            for (const auto& index : indexed_mesh.m_indices) {
                result.m_indices.emplace_back(index);
            }
        }

        return result;
    }

}
