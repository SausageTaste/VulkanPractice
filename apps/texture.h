#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "command_pool.h"


namespace dal {

    struct ImageData {
        uint32_t width, height, channels;
        VkFormat format;
        std::vector<uint8_t> buffer;
    };


    class TextureImage {

    private:
        VkImage textureImage = VK_NULL_HANDLE;
        VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
        VkFormat m_format;
        VkDeviceSize m_alloc_size = 0;
        uint32_t m_mip_levels = 1;

    public:
        void init_img(
            const char* const image_path, VkDevice logiDevice, VkPhysicalDevice physDevice,
            dal::CommandPool& cmdPool, VkQueue graphicsQ
        );
        void init_astc(
            const char* const image_path, VkDevice logiDevice, VkPhysicalDevice physDevice,
            dal::CommandPool& cmdPool, VkQueue graphicsQ
        );
        void init(
            const ImageData& image_data, VkDevice logiDevice, VkPhysicalDevice physDevice,
            dal::CommandPool& cmdPool, VkQueue graphicsQ
        );

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
