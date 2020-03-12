#pragma once

#include <vulkan/vulkan.h>


namespace dal {

    class ShaderPipeline {

    private:
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;

    public:
        void init(VkDevice device, VkRenderPass renderPass, const VkExtent2D& extent);
        void destroy(VkDevice device);

        auto getPipeline(void) const {
            return this->m_graphicsPipeline;
        }

    };

}
