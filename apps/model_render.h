#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "vert_data.h"
#include "uniform.h"


namespace dal {

    class MaterialVK {

    public:
        U_Material_InDeferred m_material_data;
        UniformBufferArray<U_Material_InDeferred> m_material_buffer;

        std::vector<DescSet> m_desc_set;
        VkImageView m_albedo_map = VK_NULL_HANDLE;

    public:
        void destroy(const VkDevice logi_device);

        void set_material(
            DescPool& pool,
            const size_t swapchain_count,
            const VkDescriptorSetLayout descriptor_set_layout,
            const UniformBufferArray<U_PerFrame_InDeferred>& uniform_buffers,
            const VkImageView texture_image_view,
            const VkSampler texture_sampler,
            const VkDevice logi_device,
            const VkPhysicalDevice phys_device
        );
        void set_material(
            DescPool& pool,
            const size_t swapchain_count,
            const VkDescriptorSetLayout descriptor_set_layout,
            const UniformBufferArray<U_PerFrame_InDeferred>& uniform_buffers,
            const VkSampler texture_sampler,
            const VkDevice logi_device,
            const VkPhysicalDevice phys_device
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

    class ModelInstance {

    private:
        Transform m_transform;

    public:
        auto& transform() {
            return this->m_transform;
        }
        auto& transform() const {
            return this->m_transform;
        }

    };


    class ModelVK {

    private:
       std::vector<RenderUnitVK> m_render_units;
       std::vector<ModelInstance> m_instances;

    public:
        void destroy(const VkDevice logi_device);

        RenderUnitVK& add_unit();
        ModelInstance& add_instance();

        auto& render_units() {
            return this->m_render_units;
        }
        auto& render_units() const {
            return this->m_render_units;
        }
        auto& instances() {
            return this->m_instances;
        }
        auto& instances() const {
            return this->m_instances;
        }

    };

}
