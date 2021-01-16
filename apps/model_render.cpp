#include "model_render.h"


namespace dal {

    void MaterialVK::destroy(const VkDevice logi_device) {
        this->m_material_buffer.destroy(logi_device);
    }

    void MaterialVK::set_material(
        const VkImageView texture_image_view,
        const VkDevice logi_device,
        const VkPhysicalDevice phys_device
    ) {
        this->m_albedo_map = texture_image_view;
        this->m_material_buffer.init(logi_device, phys_device);
        this->m_material_buffer.copy_to_buffer(this->m_material_data, logi_device);
    }

}


namespace dal {

    void RenderUnitVK::set_mesh(
        const std::vector<Vertex> vertices,
        const std::vector<uint32_t> indices,
        dal::CommandPool& cmd_pool,
        const VkDevice logi_device,
        const VkPhysicalDevice phys_device,
        const VkQueue graphics_queue
    ) {
        this->m_mesh.vertices.init(vertices, logi_device, phys_device, cmd_pool, graphics_queue);
        this->m_mesh.indices.init(indices, logi_device, phys_device, cmd_pool, graphics_queue);
    }

}


namespace dal {

    void ModelVK::DescSet2D::init(const VkDevice logi_device) {
        this->m_pool.init(128, 128, 128, 128, logi_device);
    }

    void ModelVK::DescSet2D::destroy(const VkDevice logi_device) {
        this->m_pool.destroy(logi_device);
        this->m_sets.clear();

        this->m_render_unit_count = 0;
        this->m_instance_count = 0;
    }

    DescSet& ModelVK::DescSet2D::at(const uint32_t swapchain_index, const uint32_t inst_index, const uint32_t unit_index) {
        return this->m_sets.at(this->calc_index(swapchain_index, inst_index, unit_index));
    }
    const DescSet& ModelVK::DescSet2D::at(const uint32_t swapchain_index, const uint32_t inst_index, const uint32_t unit_index) const {
        return this->m_sets.at(this->calc_index(swapchain_index, inst_index, unit_index));
    }

    void ModelVK::DescSet2D::reset(
        const std::vector<ModelInstance>& insts,
        const std::vector<RenderUnitVK>& units,
        const UniformBufferArray<U_PerFrame_InDeferred>& ubuf_per_frame_in_deferred,
        const uint32_t swapchain_count,
        const VkSampler texture_sampler,
        const VkDescriptorSetLayout desc_layout_deferred,
        const VkDevice logi_device
    ) {
        this->m_render_unit_count = units.size();
        this->m_instance_count = insts.size();

        this->m_pool.reset(logi_device);
        this->m_sets.resize(swapchain_count * insts.size() * units.size());

        for (uint32_t inst_index = 0; inst_index < insts.size(); ++inst_index) {
            for (uint32_t unit_index = 0; unit_index < units.size(); ++unit_index) {
                for (uint32_t swapchain_index = 0; swapchain_index < swapchain_count; ++swapchain_index) {
                    auto& one = this->at(swapchain_index, inst_index, unit_index);
                    one = this->m_pool.allocate(desc_layout_deferred, logi_device);
                    one.record_deferred(
                        ubuf_per_frame_in_deferred.buffer_at(swapchain_index),
                        units.at(unit_index).m_material.m_material_buffer,
                        units.at(unit_index).m_material.m_albedo_map,
                        texture_sampler,
                        logi_device
                    );
                }
            }
        }
    }

    uint32_t ModelVK::DescSet2D::calc_index(const uint32_t swapchain_index, const uint32_t inst_index, const uint32_t unit_index) const {
        return (this->m_render_unit_count * this->m_instance_count * swapchain_index) + (this->m_render_unit_count * inst_index) + unit_index;
    }

}


namespace dal {

    void ModelVK::destroy(const VkDevice logi_device) {
        this->m_desc_sets.destroy(logi_device);

        for (auto& unit : this->m_render_units) {
            unit.m_mesh.vertices.destroy(logi_device);
            unit.m_mesh.indices.destroy(logi_device);
            unit.m_material.destroy(logi_device);
        }
    }

    void ModelVK::reset_desc_sets(
        const UniformBufferArray<U_PerFrame_InDeferred>& ubuf_per_frame_in_deferred,
        const uint32_t swapchain_count,
        const VkSampler texture_sampler,
        const VkDescriptorSetLayout desc_layout_deferred,
        const VkDevice logi_device
    ) {
        this->m_desc_sets.reset(
            this->m_instances,
            this->m_render_units,
            ubuf_per_frame_in_deferred,
            swapchain_count,
            texture_sampler,
            desc_layout_deferred,
            logi_device
        );
    }

    RenderUnitVK& ModelVK::add_unit() {
        return this->m_render_units.emplace_back();
    }

    ModelInstance& ModelVK::add_instance() {
        return this->m_instances.emplace_back();
    }

}
