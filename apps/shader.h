#pragma once

#include <cassert>

#include <vulkan/vulkan.h>


namespace dal {

    class ShaderPipeline {

    private:
        VkPipelineLayout m_layout_deferred = VK_NULL_HANDLE;
        VkPipeline m_pipeline_deferred = VK_NULL_HANDLE;

        VkPipelineLayout m_layout_composition = VK_NULL_HANDLE;
        VkPipeline m_pipeline_composition = VK_NULL_HANDLE;

        VkPipelineLayout m_layout_shadow = VK_NULL_HANDLE;
        VkPipeline m_pipeline_shadow = VK_NULL_HANDLE;

    public:
        void init(
            const VkDevice device,
            const VkRenderPass renderPass,
            const VkRenderPass shadow_renderpass,
            const VkExtent2D& extent,
            const VkExtent2D& shadow_extent,
            const VkDescriptorSetLayout desc_layout_deferred,
            const VkDescriptorSetLayout desc_layout_composition,
            const VkDescriptorSetLayout desc_layout_shadow
        );
        void destroy(VkDevice device);

        auto& layout_deferred() const {
            return this->m_layout_deferred;
        }
        auto& pipeline_deferred() const {
            return this->m_pipeline_deferred;
        }

        auto& layout_composition() const {
            return this->m_layout_composition;
        }
        auto& pipeline_composition() const {
            return this->m_pipeline_composition;
        }

        auto& layout_shadow() const {
            return this->m_layout_shadow;
        }
        auto& pipeline_shadow() const {
            return this->m_pipeline_shadow;
        }

    };

}
