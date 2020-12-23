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

}
