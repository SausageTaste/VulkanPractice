#pragma once

#include <vector>

#include <vulkan/vulkan.h>


namespace dal {

    class CommandPool {

    private:
        VkCommandPool m_pool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> m_buffers;

    public:
        void init(VkPhysicalDevice physDevice, VkDevice logiDevice, VkSurfaceKHR surface, VkRenderPass renderPass,
            VkPipeline graphicsPipeline, const VkExtent2D& extent, const std::vector<VkFramebuffer>& swapChainFbufs);
        void destroy(VkDevice logiDevice);

        auto& buffers(void) const {
            return this->m_buffers;
        }

    };

}
