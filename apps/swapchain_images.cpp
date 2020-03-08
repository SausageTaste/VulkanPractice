#include "swapchain_images.h"

#include <stdexcept>


namespace dal {

    void SwapchainImages::init(VkDevice logicalDevice, VkSwapchainKHR swapChain, VkFormat swapChainImageFormat, VkExtent2D swapChainExtent) {
        // Get swap chain images.
        {
            uint32_t imageCount;
            vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
            this->m_swapChainImages.resize(imageCount);
            vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, this->m_swapChainImages.data());
        }

        // Create image views
        {
            this->m_swapChainImageViews.resize(this->m_swapChainImages.size());
            for ( size_t i = 0; i < this->m_swapChainImages.size(); i++ ) {
                VkImageViewCreateInfo createInfo = {};
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                createInfo.image = this->m_swapChainImages[i];
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                createInfo.format = swapChainImageFormat;

                createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

                createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                createInfo.subresourceRange.baseMipLevel = 0;
                createInfo.subresourceRange.levelCount = 1;
                createInfo.subresourceRange.baseArrayLayer = 0;
                createInfo.subresourceRange.layerCount = 1;

                if ( vkCreateImageView(logicalDevice, &createInfo, nullptr, &this->m_swapChainImageViews[i]) != VK_SUCCESS ) {
                    throw std::runtime_error("failed to create image views!");
                }
            }
        }
    }

    void SwapchainImages::destroy(VkDevice logicalDevice) {
        for ( auto imageView : this->m_swapChainImageViews ) {
            vkDestroyImageView(logicalDevice, imageView, nullptr);
        }
        this->m_swapChainImageViews.clear();
    }

}
