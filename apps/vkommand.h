#pragma once

#include <vector>

#include <vulkan/vulkan.h>


namespace dal {

    class CommandPool {

    private:
        VkCommandPool m_pool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> m_buffers;

    public:
        // cmdBufSize must be size of swapChainFramebuffers.
        void init(VkPhysicalDevice physDevice, VkDevice logiDevice, VkSurfaceKHR surface, const unsigned cmdBufSize);
        void destroy(VkDevice logiDevice);

    };

}
