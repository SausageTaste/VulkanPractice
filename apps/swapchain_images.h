#pragma once

#include <vector>

#include <vulkan/vulkan.h>


namespace dal {

    class SwapchainImages {

    private:
        std::vector<VkImage> m_swapChainImages;

        std::vector<VkImageView> m_swapChainImageViews;

    public:
        void init(VkDevice logicalDevice, VkSwapchainKHR swapChain, VkFormat swapChainImageFormat, VkExtent2D swapChainExtent);
        void destroy(VkDevice logicalDevice);

        auto& getViews(void) const {
            return this->m_swapChainImageViews;
        }

    };

}
