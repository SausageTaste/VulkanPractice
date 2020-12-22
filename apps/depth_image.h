#pragma once

#include <vulkan/vulkan.h>


namespace dal {

    class DepthImage {

    private:
        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;

    public:
        void init(VkExtent2D extent, VkDevice logiDevice, VkPhysicalDevice physDevice);
        void destroy(VkDevice logiDevice);

    };

}
