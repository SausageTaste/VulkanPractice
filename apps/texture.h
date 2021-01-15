#pragma once

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

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
        VkSampler textureSampler = VK_NULL_HANDLE;

    public:
        void init(VkDevice logiDevice, VkPhysicalDevice physDevice);
        void init_for_shadow_map(const VkDevice logi_device, const VkPhysicalDevice phys_device);
        void destroy(VkDevice logiDevice);

        auto& get() const {
            return this->textureSampler;
        }

    };


    class TextureManager {

    private:
        TextureSampler m_sampler1;
        TextureSampler m_sampler_shadow_map;

        std::unordered_map< std::string, std::shared_ptr<TextureUnit> > m_textures;

    public:
        void init(const VkDevice logi_device, const VkPhysicalDevice phys_device);
        void destroy(const VkDevice logi_device);

        auto& sampler_1() const {
            return this->m_sampler1;
        }
        auto& sampler_shadow_map() const {
            return this->m_sampler_shadow_map;
        }

        bool has_texture(const std::string& tex_name) const;

        std::shared_ptr<TextureUnit> request_texture(
            const char* const tex_name_ext,
            dal::CommandPool& cmd_pool,
            const VkDevice logi_device,
            const dal::PhysDevice& phys_device,
            const VkQueue graphics_queue
        );
        std::shared_ptr<TextureUnit> request_texture_astc(
            const char* const tex_name_ext,
            dal::CommandPool& cmd_pool,
            const VkDevice logi_device,
            const dal::PhysDevice& phys_device,
            const VkQueue graphics_queue
        );
        std::shared_ptr<TextureUnit> request_texture_with_mipmaps(
            const std::vector<std::string> tex_names_ext,
            dal::CommandPool& cmd_pool,
            const VkDevice logi_device,
            const dal::PhysDevice& phys_device,
            const VkQueue graphics_queue
        );
        std::shared_ptr<TextureUnit> request_texture_with_mipmaps_astc(
            const std::vector<std::string> tex_names_ext,
            dal::CommandPool& cmd_pool,
            const VkDevice logi_device,
            const dal::PhysDevice& phys_device,
            const VkQueue graphics_queue
        );

    };

}
