#include "fbufmanager.h"

#include <array>
#include <stdexcept>


namespace dal {

    void FbufManager::init(
        VkDevice device, VkRenderPass renderPass, const std::vector<VkImageView>& swapChainImageViews,
        const VkExtent2D& extent, const VkImageView depth_image_view
    ) {
        this->m_swapChainFbufs.resize(swapChainImageViews.size());

        for ( size_t i = 0; i < swapChainImageViews.size(); i++ ) {
            std::array<VkImageView, 2> attachments = {
                swapChainImageViews[i],
                depth_image_view
            };

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
