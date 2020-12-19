#include "vkommand.h"

#include <stdexcept>


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
        VkPipelineLayout pipelineLayout, const std::vector<std::vector<VkDescriptorSet>>& descriptorSetsList, const std::vector<MeshBuffer>& meshes
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

                    for (const auto& mesh : meshes) {
                        VkBuffer vertBuffers[] = {mesh.vertices.getBuf()};
                        VkDeviceSize offsets[] = {0};
                        vkCmdBindVertexBuffers(this->m_buffers[i], 0, 1, vertBuffers, offsets);
                        vkCmdBindIndexBuffer(this->m_buffers[i], mesh.indices.getBuf(), 0, VK_INDEX_TYPE_UINT16);

                        vkCmdBindDescriptorSets(
                            this->m_buffers[i],
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout,
                            0, 1, &descriptorSetsList.back()[i], 0, nullptr
                        );

                        vkCmdDrawIndexed(this->m_buffers[i], mesh.indices.size(), 1, 0, 0, 0);
                    }
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
