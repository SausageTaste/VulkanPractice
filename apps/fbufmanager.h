#pragma once

#include <array>
#include <vector>

#include <vulkan/vulkan.h>


namespace dal {

    class FbufAttachment {

    public:
        enum class Usage{ color_attachment, depth_stencil_attachment, depth_map };

    private:
        VkImage m_image = VK_NULL_HANDLE;
        VkDeviceMemory m_mem = VK_NULL_HANDLE;
        VkImageView m_view = VK_NULL_HANDLE;
        VkFormat m_format;
        uint32_t m_width = 0, m_height = 0;

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

        auto& view() const {
            return this->m_view;
        }
        auto& format() const {
            return this->m_format;
        }
        auto width() const {
            return this->m_width;
        }
        auto height() const {
            return this->m_height;
        }
        VkExtent2D extent() const {
            return { this->width(), this->height() };
        }

    };


    class GbufManager {

    private:
        class Gbuf {

        public:
            FbufAttachment m_position, m_normal, m_albedo, m_material;
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

    private:
        Gbuf m_gbuf;

    public:
        void init(
            const VkDevice logiDevice,
            const VkPhysicalDevice physDevice,
            const uint32_t width,
            const uint32_t height
        );
        void destroy(const VkDevice logiDevice);

        auto make_views_array(const VkImageView swapchain_image_view, const VkImageView depth_image_view) const {
            return std::array<VkImageView, 6>{
                swapchain_image_view,
                depth_image_view,
                this->m_gbuf.m_position.view(),
                this->m_gbuf.m_normal.view(),
                this->m_gbuf.m_albedo.view(),
                this->m_gbuf.m_material.view(),
            };
        }
        auto make_formats_array(const VkFormat swapchain_image_format, const VkFormat depth_image_format) const {
            return std::array<VkFormat, 6>{
                swapchain_image_format,
                depth_image_format,
                this->m_gbuf.m_position.format(),
                this->m_gbuf.m_normal.format(),
                this->m_gbuf.m_albedo.format(),
                this->m_gbuf.m_material.format(),
            };
        }
        auto make_views_vector(const VkImageView depth_image_view) const {
            return std::vector<VkImageView>{
                depth_image_view,
                this->m_gbuf.m_position.view(),
                this->m_gbuf.m_normal.view(),
                this->m_gbuf.m_albedo.view(),
                this->m_gbuf.m_material.view(),
            };
        }

    };


    class FbufManager {

    private:
        std::vector<VkFramebuffer> m_swapChainFbufs;

    public:
        void init(
            const VkDevice device,
            const VkRenderPass renderPass,
            const std::vector<VkImageView>& swapChainImageViews,
            const VkExtent2D& extent,
            const VkImageView depth_image_view,
            const dal::GbufManager& gbuf_man
        );
        void destroy(VkDevice device);

        auto& getList(void) const {
            return this->m_swapChainFbufs;
        }

    };


    class DepthMapManager {

    private:
        std::vector<VkFramebuffer> m_depth_fbuf;
        std::vector<FbufAttachment> m_depth_map;

    public:
        void init(
            const uint32_t count,
            const VkRenderPass render_pass,
            const VkDevice logi_device,
            const VkPhysicalDevice phys_device
        );
        void destroy(const VkDevice logi_device);

        auto& attachment(const size_t index) const {
            return this->m_depth_map.at(index);
        }
        auto& fbuf(const size_t index) const {
            return this->m_depth_fbuf.at(index);
        }

    };

}
