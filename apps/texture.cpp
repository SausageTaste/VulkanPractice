#include "texture.h"

#include <cmath>
#include <iostream>
#include <iterator>
#include <stdexcept>

#include "util_vulkan.h"


namespace {

    template <typename T>
    uint32_t calc_mip_level(const T texture_width, const T texture_height) {
        const auto a = std::floor(std::log2(std::max<T>(texture_width, texture_height)));
        return static_cast<uint32_t>(a);
    }

    void copyBufferToImage(
        VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t mip_level,
        VkDevice logiDevice, dal::CommandPool& cmdPool, VkQueue graphicsQ
    ) {
        const auto cmdBuffer = cmdPool.beginSingleTimeCmd(logiDevice);
        {
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = mip_level;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = {0, 0, 0};
            region.imageExtent = {width, height, 1};

            vkCmdCopyBufferToImage(
                cmdBuffer,
                buffer,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );
        }
        cmdPool.endSingleTimeCmd(cmdBuffer, logiDevice, graphicsQ);
    }

    void transitionImageLayout(
        VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mip_level,
        VkDevice logiDevice, dal::CommandPool& cmdPool, VkQueue graphicsQ
    ) {
        const auto cmdBuffer = cmdPool.beginSingleTimeCmd(logiDevice);
        {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = mip_level;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0; // TODO
            barrier.dstAccessMask = 0; // TODO

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;
            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            } else {
                throw std::invalid_argument("unsupported layout transition!");
            }

            vkCmdPipelineBarrier(
                cmdBuffer,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );
        }
        cmdPool.endSingleTimeCmd(cmdBuffer, logiDevice, graphicsQ);
    }

    void generateMipmaps(
        VkImage image, int32_t tex_width, int32_t tex_height, uint32_t mip_levels,
        VkDevice logiDevice, dal::CommandPool& cmdPool, VkQueue graphicsQ
    ) {
        const auto cmdBuffer = cmdPool.beginSingleTimeCmd(logiDevice);
        {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image = image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;

            int32_t mip_width = tex_width;
            int32_t mip_height = tex_height;

            for (uint32_t i = 1; i < mip_levels; ++i) {
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier(
                    cmdBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );

                VkImageBlit blit{};
                blit.srcOffsets[0] = { 0, 0, 0 };
                blit.srcOffsets[1] = { mip_width, mip_height, 1 };
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount = 1;
                blit.dstOffsets[0] = { 0, 0, 0 };
                blit.dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 };
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount = 1;

                vkCmdBlitImage(
                    cmdBuffer,
                    image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &blit,
                    VK_FILTER_LINEAR
                );

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(
                    cmdBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );

                if (mip_width > 1) mip_width /= 2;
                if (mip_height > 1) mip_height /= 2;
            }

            barrier.subresourceRange.baseMipLevel = mip_levels - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(
                cmdBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );
        }
        cmdPool.endSingleTimeCmd(cmdBuffer, logiDevice, graphicsQ);
    }


    void print_image_info(uint32_t buf_size, uint32_t alloc_size, VkFormat format) {
        std::cout << "Texture image created: raw data size is "
                  << buf_size
                  << ", vram size is "
                  << alloc_size
                  << ", format is "
                  << format
                  << std::endl;
    }

}


namespace dal {
    void TextureImage::init_img(
        const char* const image_path, VkDevice logiDevice, const dal::PhysDevice& physDevice,
        dal::CommandPool& cmdPool, VkQueue graphicsQ
    ) {
        const auto image_data = dal::open_image_stb(image_path);
        if ( physDevice.info().is_mipmap_gen_available_for(image_data.format) ) {
            this->init_gen_mipmaps(image_data, logiDevice, physDevice, cmdPool, graphicsQ);
        }
        else {
            this->init_without_mipmaps(image_data, logiDevice, physDevice, cmdPool, graphicsQ);
        }
    }

    void TextureImage::init_astc(
        const char* const image_path, VkDevice logiDevice, const dal::PhysDevice& physDevice,
        dal::CommandPool& cmdPool, VkQueue graphicsQ
    ) {
        const auto image_data = dal::open_image_astc(image_path);
        if ( physDevice.info().is_mipmap_gen_available_for(image_data.format) ) {
            this->init_gen_mipmaps(image_data, logiDevice, physDevice, cmdPool, graphicsQ);
        }
        else {
            this->init_without_mipmaps(image_data, logiDevice, physDevice, cmdPool, graphicsQ);
        }
    }

    void TextureImage::init_gen_mipmaps(
        const ImageData& image_data, VkDevice logiDevice, const dal::PhysDevice& physDevice,
        dal::CommandPool& cmdPool, VkQueue graphicsQ
    ) {
        if (!physDevice.info().is_mipmap_gen_available_for(image_data.format)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        dal::createBuffer(
            image_data.buffer.size(),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory,
            logiDevice,
            physDevice.get()
        );

        void* data;
        vkMapMemory(logiDevice, stagingBufferMemory, 0, image_data.buffer.size(), 0, &data);
        memcpy(data, image_data.buffer.data(), image_data.buffer.size());
        vkUnmapMemory(logiDevice, stagingBufferMemory);

        this->m_mip_levels = ::calc_mip_level(image_data.width, image_data.height);
        this->m_alloc_size = dal::createImage(
            image_data.width,
            image_data.height,
            this->mip_level(),
            image_data.format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage,
            textureImageMemory,
            logiDevice,
            physDevice.get()
        );

        ::transitionImageLayout(
            this->textureImage,
            image_data.format,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            this->mip_level(),
            logiDevice, cmdPool, graphicsQ
        );
        ::copyBufferToImage(
            stagingBuffer,
            this->textureImage,
            image_data.width,
            image_data.height,
            0,
            logiDevice, cmdPool, graphicsQ
        );
        ::generateMipmaps(
            this->image(),
            image_data.width,
            image_data.height,
            this->mip_level(),
            logiDevice,
            cmdPool,
            graphicsQ
        );
        std::cout << "Mipmap generated" << std::endl;

        vkDestroyBuffer(logiDevice, stagingBuffer, nullptr);
        vkFreeMemory(logiDevice, stagingBufferMemory, nullptr);

        this->m_format = image_data.format;

        ::print_image_info(image_data.buffer.size(), this->m_alloc_size, this->format());
    }

    void TextureImage::init_mipmaps(
        const std::vector<ImageData>& image_datas, VkDevice logiDevice,
        const dal::PhysDevice& physDevice, dal::CommandPool& cmdPool, VkQueue graphicsQ
    ) {
        this->m_format = image_datas[0].format;
        this->m_mip_levels = image_datas.size();
        this->m_alloc_size = dal::createImage(
            image_datas[0].width,
            image_datas[0].height,
            this->mip_level(),
            this->format(),
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage,
            textureImageMemory,
            logiDevice,
            physDevice.get()
        );

        ::transitionImageLayout(
            this->textureImage,
            this->format(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            this->mip_level(),
            logiDevice, cmdPool, graphicsQ
        );


        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
        dal::createBuffer(
            image_datas[0].buffer.size() * 2,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory,
            logiDevice,
            physDevice.get()
        );
        for (int i = 0; i < image_datas.size(); ++i) {
            void* data = nullptr;
            vkMapMemory(logiDevice, stagingBufferMemory, 0, image_datas[i].buffer.size(), 0, &data);
            memcpy(data, image_datas[i].buffer.data(), image_datas[i].buffer.size());
            vkUnmapMemory(logiDevice, stagingBufferMemory);

            ::copyBufferToImage(
                stagingBuffer,
                this->textureImage,
                image_datas[i].width,
                image_datas[i].height,
                i,
                logiDevice, cmdPool, graphicsQ
            );
        }
        vkDestroyBuffer(logiDevice, stagingBuffer, nullptr);
        vkFreeMemory(logiDevice, stagingBufferMemory, nullptr);

        ::transitionImageLayout(
            this->textureImage,
            this->format(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            this->mip_level(),
            logiDevice, cmdPool, graphicsQ
        );

        ::print_image_info(image_datas[0].buffer.size(), this->m_alloc_size, this->format());
    }

    void TextureImage::init_without_mipmaps(
        const ImageData& image_data, VkDevice logiDevice, const dal::PhysDevice& physDevice,
        dal::CommandPool& cmdPool, VkQueue graphicsQ
    ) {
        this->m_format = image_data.format;
        this->m_mip_levels = 1;
        this->m_alloc_size = dal::createImage(
            image_data.width,
            image_data.height,
            this->mip_level(),
            this->format(),
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage,
            textureImageMemory,
            logiDevice,
            physDevice.get()
        );

        ::transitionImageLayout(
            this->textureImage,
            this->format(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            this->mip_level(),
            logiDevice, cmdPool, graphicsQ
        );

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
        dal::createBuffer(
            image_data.buffer.size(),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory,
            logiDevice,
            physDevice.get()
        );

        void* data = nullptr;
        vkMapMemory(logiDevice, stagingBufferMemory, 0, image_data.buffer.size(), 0, &data);
        memcpy(data, image_data.buffer.data(), image_data.buffer.size());
        vkUnmapMemory(logiDevice, stagingBufferMemory);

        ::copyBufferToImage(
            stagingBuffer,
            this->textureImage,
            image_data.width,
            image_data.height,
            0,
            logiDevice, cmdPool, graphicsQ
        );

        vkDestroyBuffer(logiDevice, stagingBuffer, nullptr);
        vkFreeMemory(logiDevice, stagingBufferMemory, nullptr);

        ::transitionImageLayout(
            this->textureImage,
            this->format(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            this->mip_level(),
            logiDevice, cmdPool, graphicsQ
        );

        ::print_image_info(image_data.buffer.size(), this->m_alloc_size, this->format());
    }

    void TextureImage::destroy(VkDevice logiDevice) {
        if (VK_NULL_HANDLE != this->textureImage) {
            vkDestroyImage(logiDevice, this->textureImage, nullptr);
            this->textureImage = VK_NULL_HANDLE;
        }
        if (VK_NULL_HANDLE != this->textureImageMemory) {
            vkFreeMemory(logiDevice, this->textureImageMemory, nullptr);
            this->textureImageMemory = VK_NULL_HANDLE;
        }
    }

}


namespace dal {

    void TextureImageView::init(VkDevice logiDevice, VkImage textureImage, VkFormat format, uint32_t mip_level) {
        this->textureImageView = dal::createImageView(textureImage, format, mip_level, VK_IMAGE_ASPECT_COLOR_BIT, logiDevice);
    }

    void TextureImageView::destroy(VkDevice logiDevice) {
        if (VK_NULL_HANDLE != this->textureImageView) {
            vkDestroyImageView(logiDevice, this->textureImageView, nullptr);
            this->textureImageView = VK_NULL_HANDLE;
        }
    }

}


namespace dal {

    void TextureSampler::init(VkDevice logiDevice, VkPhysicalDevice physDevice) {
        this->destroy(logiDevice);

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physDevice, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

        if (VK_SUCCESS != vkCreateSampler(logiDevice, &samplerInfo, nullptr, &this->textureSampler)) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void TextureSampler::init_for_shadow_map(const VkDevice logi_device, const VkPhysicalDevice phys_device) {
        this->destroy(logi_device);

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(phys_device, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.mipLodBias = 0;
        samplerInfo.minLod = 0;
        samplerInfo.maxLod = 0;

        if (VK_SUCCESS != vkCreateSampler(logi_device, &samplerInfo, nullptr, &this->textureSampler)) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void TextureSampler::destroy(VkDevice logiDevice) {
        if (VK_NULL_HANDLE != this->textureSampler) {
            vkDestroySampler(logiDevice, this->textureSampler, nullptr);
            this->textureSampler = VK_NULL_HANDLE;
        }
    }

}


namespace dal {

    void TextureManager::init(const VkDevice logi_device, const VkPhysicalDevice phys_device) {
        this->m_sampler1.init(logi_device, phys_device);
        this->m_sampler_shadow_map.init_for_shadow_map(logi_device, phys_device);
    }

    void TextureManager::destroy(const VkDevice logi_device) {
        this->m_sampler1.destroy(logi_device);
        this->m_sampler_shadow_map.destroy(logi_device);

        for (auto& [name, tex] : this->m_textures) {
            tex->view.destroy(logi_device);
            tex->image.destroy(logi_device);
        }

        this->m_textures.clear();
    }

    bool TextureManager::has_texture(const std::string& tex_name) const {
        return this->m_textures.end() != this->m_textures.find(tex_name);
    }

    std::shared_ptr<TextureUnit> TextureManager::request_texture(
        const char* const tex_name_ext,
        dal::CommandPool& cmd_pool,
        const VkDevice logi_device,
        const dal::PhysDevice& phys_device,
        const VkQueue graphics_queue
    ) {
        const auto iter = this->m_textures.find(tex_name_ext);
        if (this->m_textures.end() != iter) {
            return iter->second;
        }

        std::shared_ptr<TextureUnit> tex;
        tex.reset(new TextureUnit);

        tex->image.init_img(
            (dal::get_res_path() + "/image/" + tex_name_ext).c_str(),
            logi_device,
            phys_device,
            cmd_pool,
            graphics_queue
        );
        tex->view.init(
            logi_device,
            tex->image.image(),
            tex->image.format(),
            tex->image.mip_level()
        );

        this->m_textures[tex_name_ext] = tex;
        return tex;
    }

    std::shared_ptr<TextureUnit> TextureManager::request_texture_astc(
        const char* const tex_name_ext,
        dal::CommandPool& cmd_pool,
        const VkDevice logi_device,
        const dal::PhysDevice& phys_device,
        const VkQueue graphics_queue
    ) {
        const auto iter = this->m_textures.find(tex_name_ext);
        if (this->m_textures.end() != iter) {
            return iter->second;
        }

        std::shared_ptr<TextureUnit> tex;
        tex.reset(new TextureUnit);

        tex->image.init_astc(
            (dal::get_res_path() + "/image/" + tex_name_ext).c_str(),
            logi_device,
            phys_device,
            cmd_pool,
            graphics_queue
        );
        tex->view.init(
            logi_device,
            tex->image.image(),
            tex->image.format(),
            tex->image.mip_level()
        );

        this->m_textures[tex_name_ext] = tex;
        return tex;
    }

    std::shared_ptr<TextureUnit> TextureManager::request_texture_with_mipmaps(
        const std::vector<std::string> tex_names_ext,
        dal::CommandPool& cmd_pool,
        const VkDevice logi_device,
        const dal::PhysDevice& phys_device,
        const VkQueue graphics_queue
    ) {
        std::vector<dal::ImageData> image_datas;

        for (const auto& tex_name_ext : tex_names_ext) {
            image_datas.emplace_back(dal::open_image_stb((dal::get_res_path() + "/image/" + tex_name_ext).c_str()));
        }

        std::shared_ptr<TextureUnit> tex;
        tex.reset(new TextureUnit);

        tex->image.init_mipmaps(
            image_datas,
            logi_device,
            phys_device,
            cmd_pool,
            graphics_queue
        );
        tex->view.init(
            logi_device,
            tex->image.image(),
            tex->image.format(),
            tex->image.mip_level()
        );

        this->m_textures[tex_names_ext[0]] = tex;
        return tex;
    }

    std::shared_ptr<TextureUnit> TextureManager::request_texture_with_mipmaps_astc(
        const std::vector<std::string> tex_names_ext,
        dal::CommandPool& cmd_pool,
        const VkDevice logi_device,
        const dal::PhysDevice& phys_device,
        const VkQueue graphics_queue
    ) {
        std::vector<dal::ImageData> image_datas;

        for (const auto& tex_name_ext : tex_names_ext) {
            image_datas.emplace_back(dal::open_image_astc((dal::get_res_path() + "/image/" + tex_name_ext).c_str()));
        }

        std::shared_ptr<TextureUnit> tex;
        tex.reset(new TextureUnit);

        tex->image.init_mipmaps(
            image_datas,
            logi_device,
            phys_device,
            cmd_pool,
            graphics_queue
        );
        tex->view.init(
            logi_device,
            tex->image.image(),
            tex->image.format(),
            tex->image.mip_level()
        );

        this->m_textures[tex_names_ext[0]] = tex;
        return tex;
    }

}
