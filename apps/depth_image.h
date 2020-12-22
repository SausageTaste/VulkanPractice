#pragma once

#include <vulkan/vulkan.h>


namespace dal {

    class DepthImage {

    private:
        VkImage depthImage = VK_NULL_HANDLE;
        VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
        VkImageView depthImageView = VK_NULL_HANDLE;

        VkFormat m_depth_format;

    public:
        void init(VkExtent2D extent, VkDevice logiDevice, VkPhysicalDevice physDevice);
        void destroy(VkDevice logiDevice);

        auto& format() const {
            return this->m_depth_format;
        }
        auto& image_view() const {
            return this->depthImageView;
        }

    };

}
