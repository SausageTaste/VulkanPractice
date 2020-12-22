#include "texture.h"

#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "util_vulkan.h"


namespace {

    void copyBufferToImage(
        VkBuffer buffer, VkImage image, uint32_t width, uint32_t height,
        VkDevice logiDevice, dal::CommandPool& cmdPool, VkQueue graphicsQ
    ) {
        const auto cmdBuffer = cmdPool.beginSingleTimeCmd(logiDevice);
        {
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
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
        VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
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
            barrier.subresourceRange.levelCount = 1;
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

}


namespace dal {

    void TextureImage::init(const char* const image_path, VkDevice logiDevice, VkPhysicalDevice physDevice, dal::CommandPool& cmdPool, VkQueue graphicsQ) {
        int img_width, img_height, img_channels;
        const auto pixels = stbi_load(
            image_path, &img_width, &img_height, &img_channels, STBI_rgb_alpha
        );
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        VkDeviceSize imageSize = img_width * img_height * 4;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        dal::createBuffer(
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory,
            logiDevice,
            physDevice
        );

        void* data;
        vkMapMemory(logiDevice, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(logiDevice, stagingBufferMemory);

        stbi_image_free(pixels);

        dal::createImage(
            img_width,
            img_height,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage,
            textureImageMemory,
            logiDevice,
            physDevice
        );

        ::transitionImageLayout(
            this->textureImage,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            logiDevice, cmdPool, graphicsQ
        );
        ::copyBufferToImage(
            stagingBuffer,
            this->textureImage,
            static_cast<uint32_t>(img_width),
            static_cast<uint32_t>(img_height),
            logiDevice, cmdPool, graphicsQ
        );
        ::transitionImageLayout(
            this->textureImage,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            logiDevice, cmdPool, graphicsQ
        );

        vkDestroyBuffer(logiDevice, stagingBuffer, nullptr);
        vkFreeMemory(logiDevice, stagingBufferMemory, nullptr);
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

    void TextureImageView::init(VkDevice logiDevice, VkImage textureImage) {
        this->textureImageView = dal::createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, logiDevice);
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
        samplerInfo.maxLod = 0.0f;

        if (VK_SUCCESS != vkCreateSampler(logiDevice, &samplerInfo, nullptr, &this->textureSampler)) {
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
