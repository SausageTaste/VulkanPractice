#include "depth_image.h"

#include <vector>
#include <stdexcept>

#include "util_vulkan.h"


namespace {

    VkFormat findSupportedFormat(
        const std::vector<VkFormat>& candidates,
        const VkImageTiling tiling,
        const VkFormatFeatureFlags features,
        const VkPhysicalDevice physDevice
    ) {
        for (const auto format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    VkFormat findDepthFormat(const VkPhysicalDevice physDevice) {
        return ::findSupportedFormat(
            {
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT
            },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
            physDevice
        );
    }

    bool hasStencilComponent(const VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

}



namespace  dal {

    void DepthImage::init(VkExtent2D extent, VkDevice logiDevice, VkPhysicalDevice physDevice) {
        this->m_depth_format = ::findDepthFormat(physDevice);

        dal::createImage(
            extent.width, extent.height,
            1,
            this->m_depth_format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            this->depthImage,
            this->depthImageMemory,
            logiDevice,
            physDevice
        );

        this->depthImageView = dal::createImageView(
            this->depthImage,
            this->m_depth_format,
            1,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            logiDevice
        );
    }

    void DepthImage::destroy(VkDevice logiDevice) {
        if (VK_NULL_HANDLE != this->depthImageView) {
            vkDestroyImageView(logiDevice, this->depthImageView, nullptr);
            this->depthImageView = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != this->depthImage) {
            vkDestroyImage(logiDevice, this->depthImage, nullptr);
            this->depthImage = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != this->depthImageMemory) {
            vkFreeMemory(logiDevice, this->depthImageMemory, nullptr);
            this->depthImageMemory = VK_NULL_HANDLE;
        }
    }

}
