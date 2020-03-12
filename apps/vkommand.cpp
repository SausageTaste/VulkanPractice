#include "vkommand.h"

#include <stdexcept>

#include "util_vulkan.h"


namespace {

    VkCommandPool createCommandPool(VkPhysicalDevice PhysDevice, VkDevice logiDevice, VkSurfaceKHR surface) {
        dal::QueueFamilyIndices queueFamilyIndices = dal::findQueueFamilies(PhysDevice, surface);

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily();
        poolInfo.flags = 0; // Optional

        VkCommandPool commandPool = VK_NULL_HANDLE;
        if ( vkCreateCommandPool(logiDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS ) {
            throw std::runtime_error("failed to create command pool!");
        }

        return commandPool;
    }
}


namespace dal {

    void CommandPool::init(VkPhysicalDevice physDevice, VkDevice logiDevice, VkSurfaceKHR surface, VkRenderPass renderPass,
        VkPipeline graphicsPipeline, const VkExtent2D& extent, const std::vector<VkFramebuffer>& swapChainFbufs)
    {
        this->m_pool = createCommandPool(physDevice, logiDevice, surface);

        // Create command buffers
        {
            this->m_buffers.resize(swapChainFbufs.size());

            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = this->m_pool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = static_cast<uint32_t>(this->m_buffers.size());

            if ( vkAllocateCommandBuffers(logiDevice, &allocInfo, this->m_buffers.data()) != VK_SUCCESS ) {
                throw std::runtime_error("failed to allocate command buffers!");
            }
        }

        // Record to buffers
        {
            for ( size_t i = 0; i < this->m_buffers.size(); i++ ) {
                VkCommandBufferBeginInfo beginInfo = {};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = 0; // Optional
                beginInfo.pInheritanceInfo = nullptr; // Optional

                if ( VK_SUCCESS != vkBeginCommandBuffer(this->m_buffers[i], &beginInfo) ) {
                    throw std::runtime_error("failed to begin recording command buffer!");
                }
                {
                    VkRenderPassBeginInfo renderPassInfo = {};
                    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    renderPassInfo.renderPass = renderPass;
                    renderPassInfo.framebuffer = swapChainFbufs[i];
                    renderPassInfo.renderArea.offset = { 0, 0 };
                    renderPassInfo.renderArea.extent = extent;
                    const VkClearValue clearColor = { 0.f, 0.f, 0.f, 1.f };
                    renderPassInfo.clearValueCount = 1;
                    renderPassInfo.pClearValues = &clearColor;

                    vkCmdBeginRenderPass(this->m_buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                    {
                        vkCmdBindPipeline(this->m_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
                        vkCmdDraw(this->m_buffers[i], 3, 1, 0, 0);
                    }
                    vkCmdEndRenderPass(this->m_buffers[i]);
                }
                if ( VK_SUCCESS != vkEndCommandBuffer(this->m_buffers[i]) ) {
                    throw std::runtime_error("failed to record command buffer!");
                }
            }
        }
    }

    void CommandPool::destroy(VkDevice logiDevice) {
        vkDestroyCommandPool(logiDevice, this->m_pool, nullptr);
        this->m_pool = VK_NULL_HANDLE;
    }

}
