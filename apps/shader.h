#pragma once

#include <cassert>

#include <vulkan/vulkan.h>


namespace dal {

    class ShaderPipeline {

    public:
        VkPipelineLayout m_layout_deferred = VK_NULL_HANDLE;
        VkPipelineLayout m_layout_composition = VK_NULL_HANDLE;
        VkPipeline m_pipeline_deferred = VK_NULL_HANDLE;
        VkPipeline m_pipeline_composition = VK_NULL_HANDLE;

    public:
        void init(
            const VkDevice device,
            const VkRenderPass renderPass,
            const VkExtent2D& extent,
            const VkDescriptorSetLayout desc_layout_deferred,
            const VkDescriptorSetLayout desc_layout_composition
        );
        void destroy(VkDevice device);

    };

}
