#include "texture.h"

#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "util_windows.h"
#include "util_vulkan.h"


namespace {

    void createImage(
        uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
        VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
        VkDeviceMemory& imageMemory, VkDevice logiDevice, VkPhysicalDevice physDevice
    ) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(logiDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(logiDevice, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = dal::findMemType(
            memRequirements.memoryTypeBits, properties, physDevice
        );

        if (vkAllocateMemory(logiDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(logiDevice, image, imageMemory, 0);
    }

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

    void TextureImage::init(VkDevice logiDevice, VkPhysicalDevice physDevice, dal::CommandPool& cmdPool, VkQueue graphicsQ) {
        int img_width, img_height, img_channels;
        const auto pixels = stbi_load(
            (dal::findResPath() + "/image/hikari.png").c_str(),
            &img_width, &img_height, &img_channels, STBI_rgb_alpha
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

        ::createImage(
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
        this->textureImageView = dal::createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, logiDevice);
    }

    void TextureImageView::destroy(VkDevice logiDevice) {
        if (VK_NULL_HANDLE != this->textureImageView) {
            vkDestroyImageView(logiDevice, this->textureImageView, nullptr);
            this->textureImageView = VK_NULL_HANDLE;
        }
    }

}
