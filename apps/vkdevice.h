#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "swapchain_images.h"


namespace dal {

    class GraphicDevice {

    private:
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_logicalDevice = VK_NULL_HANDLE;
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_presentQueue = VK_NULL_HANDLE;
        VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;

        VkFormat m_swapChainImageFormat;
        VkExtent2D m_swapChainExtent;

        SwapchainImages m_swapchainImages;

    public:
        ~GraphicDevice(void);

        void init(const VkInstance instance, const VkSurfaceKHR surface);
        void destroy(void);

    };

}
