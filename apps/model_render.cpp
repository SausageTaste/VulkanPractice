#include "model_render.h"


namespace dal {

    void MaterialVK::destroy(const VkDevice logi_device) {
        this->m_material_buffer.destroy(logi_device);
    }

    void MaterialVK::set_material(
        DescPool& pool,
        const size_t swapchain_count,
        const VkDescriptorSetLayout descriptor_set_layout,
        const UniformBufferArray<U_PerFrame_InDeferred>& uniform_buffers,
        const VkImageView texture_image_view,
        const VkSampler texture_sampler,
        const VkDevice logi_device,
        const VkPhysicalDevice phys_device
    ) {
        this->m_albedo_map = texture_image_view;

        this->m_material_buffer.init(1, logi_device, phys_device);
        this->m_material_buffer.copy_to_buffer(0, this->m_material_data, logi_device);

        this->m_desc_set = pool.allocate(swapchain_count, descriptor_set_layout, logi_device);
        for (uint32_t i = 0; i < this->m_desc_set.size(); ++i) {
            this->m_desc_set.at(i).record_deferred(
                uniform_buffers.buffer_at(i),
                this->m_material_buffer.buffer_at(0),
                this->m_albedo_map,
                texture_sampler,
                logi_device
            );
        }
    }

    void MaterialVK::set_material(
        DescPool& pool,
        const size_t swapchain_count,
        const VkDescriptorSetLayout descriptor_set_layout,
        const UniformBufferArray<U_PerFrame_InDeferred>& uniform_buffers,
        const VkSampler texture_sampler,
        const VkDevice logi_device,
        const VkPhysicalDevice phys_device
    ) {
        this->m_material_buffer.init(1, logi_device, phys_device);
        this->m_material_buffer.copy_to_buffer(0, this->m_material_data, logi_device);

        this->m_desc_set = pool.allocate(swapchain_count, descriptor_set_layout, logi_device);
        for (uint32_t i = 0; i < this->m_desc_set.size(); ++i) {
            this->m_desc_set.at(i).record_deferred(
                uniform_buffers.buffer_at(i),
                this->m_material_buffer.buffer_at(0),
                this->m_albedo_map,
                texture_sampler,
                logi_device
            );
        }
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

    DescSet& ModelVK::DescSet2D::at(const uint32_t swapchain_index, const uint32_t inst_index, const uint32_t unit_index) {
        return this->m_sets.at(this->calc_index(swapchain_index, inst_index, unit_index));
    }
    const DescSet& ModelVK::DescSet2D::at(const uint32_t swapchain_index, const uint32_t inst_index, const uint32_t unit_index) const {
        return this->m_sets.at(this->calc_index(swapchain_index, inst_index, unit_index));
    }

    void ModelVK::DescSet2D::reset(
        const std::vector<ModelInstance>& insts,
        const std::vector<RenderUnitVK>& units,
        const UniformBuffer<U_PerFrame_InDeferred>& ubuf_per_frame_in_deferred,
        const uint32_t swapchain_count,
        const VkSampler texture_sampler,
        const VkDescriptorSetLayout desc_layout_deferred,
        const VkDevice logi_device
    ) {
        this->m_render_unit_count = units.size();
        this->m_swapchain_count = swapchain_count;

        this->m_pool.reset(logi_device);
        this->m_sets.resize(swapchain_count * insts.size() * units.size());

        for (uint32_t inst_index = 0; inst_index < insts.size(); ++inst_index) {
            for (uint32_t unit_index = 0; unit_index < units.size(); ++unit_index) {
                for (uint32_t swapchain_index = 0; swapchain_index < swapchain_count; ++swapchain_index) {
                    auto& one = this->at(swapchain_index, inst_index, unit_index);
                    one = this->m_pool.allocate(desc_layout_deferred, logi_device);
                    one.record_deferred(
                        ubuf_per_frame_in_deferred,
                        units.at(unit_index).m_material.m_material_buffer.buffer_at(swapchain_index),
                        units.at(unit_index).m_material.m_albedo_map,
                        texture_sampler,
                        logi_device
                    );
                }
            }
        }
    }

    uint32_t ModelVK::DescSet2D::calc_index(const uint32_t swapchain_index, const uint32_t inst_index, const uint32_t unit_index) const {
        return (this->m_render_unit_count * this->m_swapchain_count * swapchain_index) + (this->m_render_unit_count * inst_index) + unit_index;
    }

}


namespace dal {

    void ModelVK::destroy(const VkDevice logi_device) {
        for (auto& unit : this->m_render_units) {
            unit.m_mesh.vertices.destroy(logi_device);
            unit.m_mesh.indices.destroy(logi_device);
            unit.m_material.destroy(logi_device);
        }
    }

    RenderUnitVK& ModelVK::add_unit() {
        return this->m_render_units.emplace_back();
    }

    ModelInstance& ModelVK::add_instance() {
        return this->m_instances.emplace_back();
    }

}
