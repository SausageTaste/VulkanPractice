#pragma once

#include <cassert>

#include <vulkan/vulkan.h>


namespace dal {

    class ShaderPipeline {

    private:
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

        auto& layout_deferred() const {
            return this->m_layout_deferred;
        }
        auto& layout_composition() const {
            return this->m_layout_composition;
        }
        auto& pipeline_deferred() const {
            return this->m_pipeline_deferred;
        }
        auto& pipeline_composition() const {
            return this->m_pipeline_composition;
        }

    };

}
