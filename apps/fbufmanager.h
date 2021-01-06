#pragma once

#include <vector>

#include <vulkan/vulkan.h>


namespace dal {

    class FbufManager {

    private:
        std::vector<VkFramebuffer> m_swapChainFbufs;

    public:
        void init(
            VkDevice device, VkRenderPass renderPass, const std::vector<VkImageView>& swapChainImageViews,
            const VkExtent2D& extent, const VkImageView depth_image_view
        );
        void destroy(VkDevice device);

        auto& getList(void) const {
            return this->m_swapChainFbufs;
        }

    };


    class FbufAttachment {

    public:
        enum class Usage{ color, depth_stencil };

    private:
        VkImage m_image = VK_NULL_HANDLE;
        VkDeviceMemory m_mem = VK_NULL_HANDLE;
        VkImageView m_view = VK_NULL_HANDLE;
        VkFormat m_format;

    public:
        void init(
            const VkDevice logiDevice,
            const VkPhysicalDevice physDevice,
            const VkFormat format,
            const FbufAttachment::Usage usage,
            const uint32_t width,
            const uint32_t height
        );
        void destroy(const VkDevice logiDevice);

    };


    class GbufManager {

    private:
        class Gbuf {

        public:
            FbufAttachment m_position, m_normal, m_albedo;
            uint32_t m_width, m_height;

        public:
            void init(
                const VkDevice logiDevice,
                const VkPhysicalDevice physDevice,
                const uint32_t width,
                const uint32_t height
            );
            void destroy(const VkDevice logiDevice);

        };

    public:
        std::vector<Gbuf> m_gbuf;

    public:
        void init(
            const VkDevice logiDevice,
            const VkPhysicalDevice physDevice,
            const size_t swapchain_count,
            const uint32_t width,
            const uint32_t height
        );
        void destroy(const VkDevice logiDevice);

        auto& at(const size_t index) const {
            return this->m_gbuf.at(index);
        }

    };

}
