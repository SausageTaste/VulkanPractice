#include "swapchain_images.h"

#include <cassert>
#include <stdexcept>

#include "util_vulkan.h"


namespace dal {

    void SwapchainImages::init(VkDevice logicalDevice, VkSwapchainKHR swapChain, VkFormat swapChainImageFormat, VkExtent2D swapChainExtent) {
        // Get swap chain images.
        {
            uint32_t imageCount;
            vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
            this->m_images.resize(imageCount);
            vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, this->m_images.data());
        }

        // Create image views
        {
            this->m_views.resize(this->m_images.size());
            for ( size_t i = 0; i < this->m_images.size(); i++ ) {
                this->m_views[i] = dal::createImageView(
                    this->m_images[i],
                    swapChainImageFormat,
                    1,
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    logicalDevice
                );
            }
        }
    }

    void SwapchainImages::destroy(VkDevice logicalDevice) {
        for ( auto imageView : this->m_views ) {
            vkDestroyImageView(logicalDevice, imageView, nullptr);
        }
        this->m_views.clear();
    }

    size_t SwapchainImages::size(void) const {
        assert(this->m_images.size() == this->m_views.size());
        return this->m_images.size();
    }

}
