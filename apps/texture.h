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

        auto& image() const {
            return this->textureImage;
        }

    };


    class TextureImageView {

    private:
        VkImageView textureImageView;

    public:
        void init(VkDevice logiDevice, VkImage textureImage);
        void destroy(VkDevice logiDevice);

        auto& get() const {
            return this->textureImageView;
        }

    };


    class TextureSampler {

    private:
        VkSampler textureSampler;

    public:
        void init(VkDevice logiDevice, VkPhysicalDevice physDevice);
        void destroy(VkDevice logiDevice);

        auto& get() const {
            return this->textureSampler;
        }

    };

}
