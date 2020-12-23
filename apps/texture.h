#pragma once

#include <vulkan/vulkan.h>

#include "command_pool.h"


namespace dal {

    class TextureImage {

    private:
        VkImage textureImage;
        VkDeviceMemory textureImageMemory;
        VkFormat m_format;

    public:
        void init(const char* const image_path, VkDevice logiDevice, VkPhysicalDevice physDevice, dal::CommandPool& cmdPool, VkQueue graphicsQ);
        void destroy(VkDevice logiDevice);

        auto& image() const {
            return this->textureImage;
        }
        auto& format() const {
            return this->m_format;
        }

    };


    class TextureImageView {

    private:
        VkImageView textureImageView;

    public:
        void init(VkDevice logiDevice, VkImage textureImage, VkFormat format);
        void destroy(VkDevice logiDevice);

        auto& get() const {
            return this->textureImageView;
        }

    };


    struct TextureUnit {
        TextureImage image;
        TextureImageView view;
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
