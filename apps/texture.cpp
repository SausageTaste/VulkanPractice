#include "texture.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "util_vulkan.h"


namespace amd {

    /// Header for the on-disk format generated by astcenc.
    struct ASTCHeader
    {
        /// Magic value
        uint8_t magic[4];
        /// Block size in X
        uint8_t blockdimX;
        /// Block size in Y
        uint8_t blockdimY;
        /// Block size in Z
        uint8_t blockdimZ;
        /// Size of the image in pixels (X), least significant byte first.
        uint8_t xsize[3];
        /// Size of the image in pixels (Y), least significant byte first.
        uint8_t ysize[3];
        /// Size of the image in pixels (Z), least significant byte first.
        uint8_t zsize[3];
    };
    static_assert(sizeof(ASTCHeader) == 16, "Packed ASTC header struct is not 16 bytes.");

    #define ASTC_MAGIC 0x5CA1AB13

    bool loadASTCTextureFromAsset(
        const char* const pPath, const std::vector<uint8_t>& compressed, std::vector<uint8_t> *pBuffer, unsigned *pWidth, unsigned *pHeight, VkFormat *pFormat
    ) {
        if (compressed.size() < sizeof(ASTCHeader))
            return false;

        ASTCHeader header;
        memcpy(&header, compressed.data(), sizeof(ASTCHeader));
        uint32_t magic = header.magic[0] | (uint32_t(header.magic[1]) << 8) | (uint32_t(header.magic[2]) << 16) |
                        (uint32_t(header.magic[3]) << 24);

        if (magic != ASTC_MAGIC)
        {
            //LOGE("Texture %s is not ASTC.\n", pPath);
            return false;
        }

        if (header.blockdimZ != 1)
        {
            //LOGE("ASTC 3D textures not supported yet in Vulkan.\n");
            return false;
        }

        if (header.blockdimX == 4 && header.blockdimY == 4) // 4x4
            *pFormat = VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
        else if (header.blockdimX == 5 && header.blockdimY == 4) // 5x4
            *pFormat = VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
        else if (header.blockdimX == 5 && header.blockdimY == 5) // 5x5
            *pFormat = VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
        else if (header.blockdimX == 6 && header.blockdimY == 5) // 6x5
            *pFormat = VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
        else if (header.blockdimX == 6 && header.blockdimY == 6) // 6x6
            *pFormat = VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
        else if (header.blockdimX == 8 && header.blockdimY == 5) // 8x5
            *pFormat = VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
        else if (header.blockdimX == 8 && header.blockdimY == 6) // 8x6
            *pFormat = VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
        else if (header.blockdimX == 8 && header.blockdimY == 8) // 8x8
            *pFormat = VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
        else if (header.blockdimX == 10 && header.blockdimY == 5) // 10x5
            *pFormat = VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
        else if (header.blockdimX == 10 && header.blockdimY == 6) // 10x6
            *pFormat = VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
        else if (header.blockdimX == 10 && header.blockdimY == 8) // 10x8
            *pFormat = VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
        else if (header.blockdimX == 10 && header.blockdimY == 10) // 10x10
            *pFormat = VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
        else if (header.blockdimX == 12 && header.blockdimY == 10) // 12x10
            *pFormat = VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
        else if (header.blockdimX == 12 && header.blockdimY == 12) // 12x12
            *pFormat = VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
        else
        {
            //LOGE("Unknown ASTC block size %u x %u.\n", header.blockdimX, header.blockdimY);
            return false;
        }

        pBuffer->clear();
        pBuffer->insert(end(*pBuffer), begin(compressed) + sizeof(ASTCHeader), end(compressed));
        *pWidth = header.xsize[0] | (header.xsize[1] << 8) | (header.xsize[2] << 16);
        *pHeight = header.ysize[0] | (header.ysize[1] << 8) | (header.ysize[2] << 16);
        return true;
    }

}


namespace {

    template <typename T>
    uint32_t calc_mip_level(const T texture_width, const T texture_height) {
        const auto a = std::floor(std::log2(std::max<T>(texture_width, texture_height)));
        return static_cast<uint32_t>(a);
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


    dal::ImageData open_image_stb(const char* const image_path) {
        dal::ImageData result;

        int img_width, img_height, img_channels;
        const auto pixels = stbi_load(
            image_path, &img_width, &img_height, &img_channels, STBI_rgb_alpha
        );

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        result.width = img_width;
        result.height = img_height;
        result.channels = 4;
        result.format = VK_FORMAT_R8G8B8A8_SRGB;

        const auto imageSize = img_width * img_height * 4;
        result.buffer.resize(imageSize);
        memcpy(result.buffer.data(), pixels, static_cast<size_t>(imageSize));
        stbi_image_free(pixels);

        return result;
    }

    dal::ImageData open_image_astc(const char* const image_path) {
        dal::ImageData result;

        std::ifstream input(image_path, std::ios::binary);
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});

        unsigned int width, height;
        VkFormat format;
        if (!amd::loadASTCTextureFromAsset(image_path, buffer, &result.buffer, &width, &height, &format)) {
            throw std::runtime_error{"failed to decompress ASTC file"};
        }

        result.width = width;
        result.height = height;
        result.format = format;

        return result;
    }

}


namespace dal {
    void TextureImage::init_img(
        const char* const image_path, VkDevice logiDevice, const dal::PhysDevice& physDevice,
        dal::CommandPool& cmdPool, VkQueue graphicsQ
    ) {
        const auto image_data = ::open_image_stb(image_path);
        this->init(image_data, logiDevice, physDevice, cmdPool, graphicsQ);
    }

    void TextureImage::init_astc(
        const char* const image_path, VkDevice logiDevice, const dal::PhysDevice& physDevice,
        dal::CommandPool& cmdPool, VkQueue graphicsQ
    ) {
        const auto image_data = ::open_image_astc(image_path);
        this->init(image_data, logiDevice, physDevice, cmdPool, graphicsQ);
    }

    void TextureImage::init(
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

        vkDestroyBuffer(logiDevice, stagingBuffer, nullptr);
        vkFreeMemory(logiDevice, stagingBufferMemory, nullptr);

        this->m_format = image_data.format;

        std::cout << "Texture image created: raw data size is "
                  << image_data.buffer.size()
                  << ", vram size is "
                  << this->m_alloc_size
                  << ", format is "
                  << this->format()
                  << std::endl;
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
        samplerInfo.maxLod = 128.0f;

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
