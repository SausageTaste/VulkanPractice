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

    void CommandPool::init(VkPhysicalDevice physDevice, VkDevice logiDevice, VkSurfaceKHR surface) {
        this->destroy(logiDevice);

        this->m_pool = createCommandPool(physDevice, logiDevice, surface);
    }

    void CommandPool::destroy(const VkDevice logiDevice) {
        if (VK_NULL_HANDLE != this->m_pool) {
            vkDestroyCommandPool(logiDevice, this->m_pool, nullptr);
            this->m_pool = VK_NULL_HANDLE;
        }
    }

}


namespace dal {

    void CommandBuffers::init(VkDevice logiDevice, const std::vector<VkFramebuffer>& swapChainFbufs, VkCommandPool cmdPool) {
        this->destroy(logiDevice, cmdPool);

        // Create command buffers
        {
            this->m_buffers.resize(swapChainFbufs.size());

            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = cmdPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = static_cast<uint32_t>(this->m_buffers.size());

            if ( vkAllocateCommandBuffers(logiDevice, &allocInfo, this->m_buffers.data()) != VK_SUCCESS ) {
                throw std::runtime_error("failed to allocate command buffers!");
            }
        }
    }

    void CommandBuffers::record(
        VkRenderPass renderPass, VkPipeline graphicsPipeline, const VkExtent2D& extent, const std::vector<VkFramebuffer>& swapChainFbufs,
        VkPipelineLayout pipelineLayout, const std::vector<VkDescriptorSet>& descriptorSets, const IndexedVertices& indexed_vertices
    ) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        const VkClearValue clearColor = { 0.f, 0.f, 0.f, 1.f };
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = extent;
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        for ( size_t i = 0; i < this->m_buffers.size(); i++ ) {
            if ( VK_SUCCESS != vkBeginCommandBuffer(this->m_buffers[i], &beginInfo) ) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }
            {
                renderPassInfo.framebuffer = swapChainFbufs[i];

                vkCmdBeginRenderPass(this->m_buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                {
                    vkCmdBindPipeline(this->m_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

                    VkBuffer vertBuffers[] = {indexed_vertices.vert_buffer};
                    VkDeviceSize offsets[] = {0};
                    vkCmdBindVertexBuffers(this->m_buffers[i], 0, 1, vertBuffers, offsets);
                    vkCmdBindIndexBuffer(this->m_buffers[i], indexed_vertices.index_buffer, 0, VK_INDEX_TYPE_UINT16);

                    vkCmdBindDescriptorSets(
                        this->m_buffers[i],
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipelineLayout,
                        0, 1, &descriptorSets[i], 0, nullptr
                    );

                    vkCmdDrawIndexed(this->m_buffers[i], indexed_vertices.index_buffer_size, 1, 0, 0, 0);
                }
                vkCmdEndRenderPass(this->m_buffers[i]);
            }
            if ( VK_SUCCESS != vkEndCommandBuffer(this->m_buffers[i]) ) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
    }

    void CommandBuffers::destroy(const VkDevice logiDevice, const VkCommandPool cmdPool) {
        if (0 != this->m_buffers.size()) {
            vkFreeCommandBuffers(logiDevice, cmdPool, this->m_buffers.size(), this->m_buffers.data());
            this->m_buffers.clear();
        }
    }

}
