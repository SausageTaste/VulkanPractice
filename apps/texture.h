#pragma once

#include <vulkan/vulkan.h>

#include "command_pool.h"


namespace dal {

    class TextureImage {

    private:
        VkImage textureImage;
        VkDeviceMemory textureImageMemory;

    public:
        void init(VkDevice logiDevice, VkPhysicalDevice physDevice, dal::CommandPool& cmdPool, VkQueue graphicsQ);
        void destroy(VkDevice logiDevice);

    };

}
