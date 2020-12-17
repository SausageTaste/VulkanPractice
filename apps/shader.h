#pragma once

#include <cassert>

#include <vulkan/vulkan.h>


namespace dal {

    class ShaderPipeline {

    private:
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;

    public:
        void init(VkDevice device, VkRenderPass renderPass, const VkExtent2D& extent, VkDescriptorSetLayout descriptorSetLayout);
        void destroy(VkDevice device);

        auto getPipeline(void) const {
            assert(VK_NULL_HANDLE != this->m_graphicsPipeline);
            return this->m_graphicsPipeline;
        }
        auto& layout() const {
            assert(VK_NULL_HANDLE != this->m_pipelineLayout);
            return this->m_pipelineLayout;
        }

    };

}
