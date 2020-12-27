#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "command_pool.h"
#include "physdevice.h"
#include "util_windows.h"


namespace dal {

    class TextureImage {

    private:
        VkImage textureImage = VK_NULL_HANDLE;
        VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
        VkFormat m_format;
        VkDeviceSize m_alloc_size = 0;
        uint32_t m_mip_levels = 1;

    public:
        void init_img(
            const char* const image_path, VkDevice logiDevice, const dal::PhysDevice& physDevice,
            dal::CommandPool& cmdPool, VkQueue graphicsQ
        );
        void init_astc(
            const char* const image_path, VkDevice logiDevice, const dal::PhysDevice& physDevice,
            dal::CommandPool& cmdPool, VkQueue graphicsQ
        );
        void init_gen_mipmaps(
            const ImageData& image_data, VkDevice logiDevice, const dal::PhysDevice& physDevice,
            dal::CommandPool& cmdPool, VkQueue graphicsQ
        );
        void init_without_mipmaps(
            const ImageData& image_data, VkDevice logiDevice, const dal::PhysDevice& physDevice,
            dal::CommandPool& cmdPool, VkQueue graphicsQ
        );
        void init_mipmaps(
            const std::vector<ImageData>& image_datas, VkDevice logiDevice,
            const dal::PhysDevice& physDevice, dal::CommandPool& cmdPool, VkQueue graphicsQ
        );

        void destroy(VkDevice logiDevice);

        auto& image() const {
            return this->textureImage;
        }
        auto& format() const {
            return this->m_format;
        }
        auto& mip_level() const {
            return this->m_mip_levels;
        }

    };


    class TextureImageView {

    private:
        VkImageView textureImageView;

    public:
        void init(VkDevice logiDevice, VkImage textureImage, VkFormat format, uint32_t mip_level);
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
