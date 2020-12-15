#pragma once

#include <vulkan/vulkan.h>


namespace dal {

    class Swapchain {

    private:
        VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;

        VkFormat m_imageFormat;
        VkExtent2D m_extent;

    public:
        void init(VkSurfaceKHR surface, VkPhysicalDevice physDevice, VkDevice logiDevice, const unsigned w, const unsigned h);
        void destroy(VkDevice logiDevice);

        VkSwapchainKHR get(void) {
            return this->m_swapChain;
        }
        VkFormat imageFormat(void) const {
            return this->m_imageFormat;
        }
        VkExtent2D extent(void) const {
            return this->m_extent;
        }

    };

}
