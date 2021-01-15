#include "fbufmanager.h"

#include <array>
#include <tuple>
#include <cassert>
#include <stdexcept>

#include "util_vulkan.h"


namespace dal {

    void FbufManager::init(
        const VkDevice device,
        const VkRenderPass renderPass,
        const std::vector<VkImageView>& swapChainImageViews,
        const VkExtent2D& extent,
        const VkImageView depth_image_view,
        const dal::GbufManager& gbuf_man
    ) {
        this->m_swapChainFbufs.resize(swapChainImageViews.size());

        for ( size_t i = 0; i < swapChainImageViews.size(); i++ ) {
            const auto attachments = gbuf_man.make_views_array(swapChainImageViews[i], depth_image_view);

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = attachments.size();
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            framebufferInfo.layers = 1;

            if ( vkCreateFramebuffer(device, &framebufferInfo, nullptr, &this->m_swapChainFbufs[i]) != VK_SUCCESS ) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void FbufManager::destroy(VkDevice device) {
        for ( auto framebuffer : this->m_swapChainFbufs ) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        this->m_swapChainFbufs.clear();
    }

}


namespace {

    auto interpret_usage(const dal::FbufAttachment::Usage usage) {
        VkImageAspectFlags aspect_mask;
        VkImageLayout image_layout;
        VkImageUsageFlags flag;

        switch (usage) {
        case dal::FbufAttachment::Usage::color_attachment:
            aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            flag = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            break;
        case dal::FbufAttachment::Usage::depth_map:
            aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
            image_layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            flag =  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            break;
        case dal::FbufAttachment::Usage::depth_stencil_attachment:
            aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            flag = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            break;
        default:
            throw std::runtime_error{ "WTF" };
        }

        return std::make_tuple(flag, aspect_mask, image_layout);
    }

}

namespace dal {

    void FbufAttachment::init(
            const VkDevice logiDevice,
            const VkPhysicalDevice physDevice,
            const VkFormat format,
            const FbufAttachment::Usage usage,
            const uint32_t width,
            const uint32_t height
    ) {
        this->destroy(logiDevice);

        const auto [usage_flag, aspect_mask, image_layout] = ::interpret_usage(usage);
        this->m_format = format;
        this->m_width = width;
        this->m_height = height;

        VkImageCreateInfo image{};
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = format;
		image.extent.width = width;
		image.extent.height = height;
		image.extent.depth = 1;
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		image.usage = usage_flag;
		image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (VK_SUCCESS != vkCreateImage(logiDevice, &image, nullptr, &this->m_image)) {
            throw std::runtime_error{ "failed to create an image for a fbuf attachment" };
        }

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(logiDevice, this->m_image, &memReqs);

        VkMemoryAllocateInfo memAlloc{};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = dal::findMemType(
            memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physDevice
        );

        if (VK_SUCCESS != vkAllocateMemory(logiDevice, &memAlloc, nullptr, &this->m_mem)) {
            throw std::runtime_error{""};
        }
        if (VK_SUCCESS != vkBindImageMemory(logiDevice, this->m_image, this->m_mem, 0)) {
            throw std::runtime_error{""};
        }

        VkImageViewCreateInfo imageView{};
        imageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageView.format = format;
        imageView.components.r = VK_COMPONENT_SWIZZLE_R;
        imageView.components.g = VK_COMPONENT_SWIZZLE_G;
        imageView.components.b = VK_COMPONENT_SWIZZLE_B;
        imageView.components.a = VK_COMPONENT_SWIZZLE_A;
		imageView.subresourceRange = {};
		imageView.subresourceRange.aspectMask = aspect_mask;
		imageView.subresourceRange.baseMipLevel = 0;
		imageView.subresourceRange.levelCount = 1;
		imageView.subresourceRange.baseArrayLayer = 0;
		imageView.subresourceRange.layerCount = 1;
		imageView.image = this->m_image;

        if (VK_SUCCESS != vkCreateImageView(logiDevice, &imageView, nullptr, &this->m_view)) {
            throw std::runtime_error{""};
        }
    }

    void FbufAttachment::destroy(const VkDevice logiDevice) {
        if (VK_NULL_HANDLE != this->m_view) {
            vkDestroyImageView(logiDevice, this->m_view, nullptr);
            this->m_view = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != this->m_image) {
            vkDestroyImage(logiDevice, this->m_image, nullptr);
            this->m_image = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != this->m_mem) {
            vkFreeMemory(logiDevice, this->m_mem, nullptr);
            this->m_mem = VK_NULL_HANDLE;
        }
    }

}


namespace dal {


    void GbufManager::Gbuf::init(
        const VkDevice logiDevice,
        const VkPhysicalDevice physDevice,
        const uint32_t width,
        const uint32_t height
    ) {
        this->destroy(logiDevice);

        this->m_width = width;
        this->m_height = height;

        this->m_position.init(
            logiDevice, physDevice,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            FbufAttachment::Usage::color_attachment,
            width, height
        );
        this->m_normal.init(
            logiDevice, physDevice,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            FbufAttachment::Usage::color_attachment,
            width, height
        );
        this->m_albedo.init(
            logiDevice,
            physDevice,
            VK_FORMAT_R8G8B8A8_UNORM,
            FbufAttachment::Usage::color_attachment,
            width, height
        );
        this->m_material.init(
            logiDevice,
            physDevice,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            FbufAttachment::Usage::color_attachment,
            width, height
        );
    }

    void GbufManager::Gbuf::destroy(const VkDevice logiDevice) {
        this->m_position.destroy(logiDevice);
        this->m_normal.destroy(logiDevice);
        this->m_albedo.destroy(logiDevice);
        this->m_material.destroy(logiDevice);
    }


    void GbufManager::init(
        const VkDevice logiDevice,
        const VkPhysicalDevice physDevice,
        const uint32_t width,
        const uint32_t height
    ) {
        this->m_gbuf.init(logiDevice, physDevice, width, height);
    }

    void GbufManager::destroy(const VkDevice logiDevice) {
        this->m_gbuf.destroy(logiDevice);
    }

}


namespace dal {

    void DepthMapManager::init(
        const uint32_t count,
        const VkRenderPass render_pass,
        const VkDevice logi_device,
        const VkPhysicalDevice phys_device
    ) {
        assert(0 != count);
        assert(VK_NULL_HANDLE != render_pass);
        assert(VK_NULL_HANDLE != logi_device);
        assert(VK_NULL_HANDLE != phys_device);

        this->m_depth_map.resize(count);
        for (auto& x : this->m_depth_map) {
            x.init(logi_device, phys_device, VK_FORMAT_D32_SFLOAT, FbufAttachment::Usage::depth_map, 1024*4, 1024*4);
        }

        this->m_depth_fbuf.resize(count);
        for ( size_t i = 0; i < count; i++ ) {
            const auto view = this->m_depth_map.at(i).view();

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = render_pass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = &view;
            framebufferInfo.width = this->m_depth_map.at(i).width();
            framebufferInfo.height = this->m_depth_map.at(i).height();
            framebufferInfo.layers = 1;

            if (VK_SUCCESS != vkCreateFramebuffer(logi_device, &framebufferInfo, nullptr, &this->m_depth_fbuf.at(i))) {
                throw std::runtime_error("failed to create framebuffer for depth map");
            }
        }
    }

    void DepthMapManager::destroy(const VkDevice logi_device) {
        for (auto& x : this->m_depth_map) {
            x.destroy(logi_device);
        }
        this->m_depth_map.clear();

        for (auto& x : this->m_depth_fbuf) {
            vkDestroyFramebuffer(logi_device, x, nullptr);
        }
        this->m_depth_fbuf.clear();
    }

    std::vector<VkImageView> DepthMapManager::views() const {
        std::vector<VkImageView> result;

        for (const auto& x : this->m_depth_map) {
            result.push_back(x.view());
        }

        return result;
    }

}
