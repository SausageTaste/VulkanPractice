#include "model_render.h"


namespace dal {

    void MaterialVK::destroy(const VkDevice logi_device) {
        this->m_material_buffer.destroy(logi_device);
    }

    void MaterialVK::set_material(
        const VkDescriptorPool pool,
        const size_t swapchain_count,
        const VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkBuffer>& uniform_buffers,
        const VkImageView texture_image_view,
        const VkSampler texture_sampler,
        const VkDevice logi_device,
        const VkPhysicalDevice phys_device
    ) {
        this->m_albedo_map = texture_image_view;

        this->m_material_buffer.init(this->m_material_data, logi_device, phys_device);

        this->m_desc_set.init(swapchain_count, descriptor_set_layout, pool, logi_device);
        this->m_desc_set.record_deferred(uniform_buffers, this->m_material_buffer, this->m_albedo_map, texture_sampler, logi_device);
    }

    void MaterialVK::set_material(
        const VkDescriptorPool pool,
        const size_t swapchain_count,
        const VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkBuffer>& uniform_buffers,
        const VkSampler texture_sampler,
        const VkDevice logi_device,
        const VkPhysicalDevice phys_device
    ) {
        this->m_material_buffer.init(this->m_material_data, logi_device, phys_device);

        this->m_desc_set.init(swapchain_count, descriptor_set_layout, pool, logi_device);
        this->m_desc_set.record_deferred(uniform_buffers, this->m_material_buffer, this->m_albedo_map, texture_sampler, logi_device);
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

    void ModelVK::destroy(const VkDevice logi_device, const VkDescriptorPool pool) {
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
