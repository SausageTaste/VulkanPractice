#include "swapchain.h"

#include <vector>

#include "konst.h"
#include "util_vulkan.h"


namespace {

    template <typename T>
    constexpr T clamp(const T v, const T minv, const T maxv) {
        return max(minv, min(maxv, v));
    }


    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for ( const auto& availableFormat : availableFormats ) {
            if ( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for ( const auto& availablePresentMode : availablePresentModes ) {
            // This is the triple buffering.
            if ( VK_PRESENT_MODE_MAILBOX_KHR == availablePresentMode ) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if ( capabilities.currentExtent.width != UINT32_MAX ) {
            return capabilities.currentExtent;
        }
        else {
            VkExtent2D actualExtent = { dal::WIN_WIDTH, dal::WIN_HEIGHT };

            actualExtent.width = clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

}

// Swapchain
namespace dal {

    void Swapchain::init(VkSurfaceKHR surface, VkPhysicalDevice physDevice, VkDevice logiDevice) {
        const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(surface, physDevice);

        const VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        const VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        const VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        // Simply sticking to this minimum means that we may sometimes have to wait on the driver to complete
        // internal operations before we can acquire another image to render to. Therefore it is recommended to
        // request at least one more image than the minimum.
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if ( swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount ) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        // Create swap chain
        {
            VkSwapchainCreateInfoKHR createInfo = {};

            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = surface;

            createInfo.minImageCount = imageCount;
            createInfo.imageFormat = surfaceFormat.format;
            createInfo.imageColorSpace = surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            QueueFamilyIndices indices = findQueueFamilies(physDevice, surface);
            uint32_t queueFamilyIndices[] = { indices.m_graphicsFamily.value(), indices.m_presentFamily.value() };

            if ( indices.m_graphicsFamily != indices.m_presentFamily ) {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = 2;
                createInfo.pQueueFamilyIndices = queueFamilyIndices;
            }
            else {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                createInfo.queueFamilyIndexCount = 0; // Optional
                createInfo.pQueueFamilyIndices = nullptr; // Optional
            }

            createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            createInfo.presentMode = presentMode;
            createInfo.clipped = VK_TRUE;
            createInfo.oldSwapchain = VK_NULL_HANDLE;

            if ( vkCreateSwapchainKHR(logiDevice, &createInfo, nullptr, &this->m_swapChain) != VK_SUCCESS ) {
                throw std::runtime_error("failed to create swap chain!");
            }
        }

        this->m_imageFormat = surfaceFormat.format;
        this->m_extent = extent;
    }

    void Swapchain::destroy(VkDevice logiDevice) {
        vkDestroySwapchainKHR(logiDevice, this->m_swapChain, nullptr);
        this->m_swapChain = VK_NULL_HANDLE;
    }

}
