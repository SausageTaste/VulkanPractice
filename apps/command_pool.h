#pragma once

#include <cassert>

#include <vulkan/vulkan.h>


namespace dal {

    class CommandPool {

    private:
        VkCommandPool m_pool = VK_NULL_HANDLE;

    public:
        void init(VkPhysicalDevice physDevice, VkDevice logiDevice, VkSurfaceKHR surface);
        void destroy(const VkDevice logiDevice);

        VkCommandBuffer beginSingleTimeCmd(const VkDevice logiDevice);
        void endSingleTimeCmd(VkCommandBuffer cmdBuffer, VkDevice logiDevice, VkQueue graphicsQueue);

        auto& pool() const {
            assert(VK_NULL_HANDLE != this->m_pool);
            return this->m_pool;
        }

    };

}
