#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "vert_data.h"
#include "uniform.h"


namespace dal {

    class MaterialVK {

    public:
        DescriptorSet m_desc_set;
        VkImageView m_albedo_map = VK_NULL_HANDLE;

        void set_material(
            const VkDescriptorPool pool,
            const size_t swapchain_count,
            const VkDescriptorSetLayout descriptor_set_layout,
            const std::vector<VkBuffer>& uniform_buffers,
            const VkImageView texture_image_view,
            const VkSampler texture_sampler,
            const VkDevice logi_device
        );
        void set_material(
            const VkDescriptorPool pool,
            const size_t swapchain_count,
            const VkDescriptorSetLayout descriptor_set_layout,
            const std::vector<VkBuffer>& uniform_buffers,
            const VkSampler texture_sampler,
            const VkDevice logi_device
        );

    };


    class RenderUnitVK {

    public:
        MeshBuffer m_mesh;
        MaterialVK m_material;

    public:
        void set_mesh(
            const std::vector<Vertex> vertices,
            const std::vector<uint32_t> indices,
            dal::CommandPool& cmd_pool,
            const VkDevice logi_device,
            const VkPhysicalDevice phys_device,
            const VkQueue graphics_queue
        );

    };


    class ModelVK {

    private:
       std::vector<RenderUnitVK> m_render_units;

    public:
        void destroy(const VkDevice logi_device, const VkDescriptorPool pool);

        RenderUnitVK& add_unit();

        auto& render_units() {
            return this->m_render_units;
        }
        auto& render_units() const {
            return this->m_render_units;
        }

    };

}
