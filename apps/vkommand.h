#pragma once

#include <vector>

#include <vulkan/vulkan.h>


namespace dal {

    class CommandPool {

    private:
        VkCommandPool m_pool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> m_buffers;

    public:
        void initPool(VkPhysicalDevice physDevice, VkDevice logiDevice, VkSurfaceKHR surface);
        void initCmdBuffers(
            VkDevice logiDevice, VkRenderPass renderPass, VkPipeline graphicsPipeline,
            const VkExtent2D& extent, const std::vector<VkFramebuffer>& swapChainFbufs,
            const VkBuffer vertBuf, const uint32_t vertSize, const VkBuffer indexBuffer, const uint32_t indexSize,
            VkPipelineLayout pipelineLayout, const std::vector<VkDescriptorSet>& descriptorSets
        );
        void destroy(VkDevice logiDevice);

        auto& pool() const {
            return this->m_pool;
        }
        auto& buffers(void) const {
            return this->m_buffers;
        }

    };

}
