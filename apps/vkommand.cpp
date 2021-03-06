#include "vkommand.h"

#include <array>
#include <stdexcept>


namespace dal {

    void CommandBuffers::init(
        const VkDevice logiDevice,
        const size_t swapchain_count,
        const VkCommandPool cmdPool
    ) {
        this->destroy(logiDevice, cmdPool);

        // Create command buffers
        {
            this->m_buffers.resize(swapchain_count);

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
        const VkRenderPass renderPass,
        const VkPipeline pipeline_deferred,
        const VkPipeline pipeline_composition,
        const VkPipelineLayout pipelayout_deferred,
        const VkPipelineLayout pipelayout_composition,
        const VkExtent2D& extent,
        const std::vector<VkFramebuffer>& swapChainFbufs,
        const std::vector<std::vector<VkDescriptorSet>>& descset_composition,
        const std::vector<ModelVK>& models
    ) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        std::array<VkClearValue, 6> clear_values{};
        clear_values[0].color = {0.f, 0.f, 0.f, 1.f};
        clear_values[1].depthStencil = {1.f, 0};
        clear_values[2].color = {0.f, 0.f, 0.f, 1.f};
        clear_values[3].color = {0.f, 0.f, 0.f, 1.f};
        clear_values[4].color = {0.f, 0.f, 0.f, 1.f};
        clear_values[5].color = {0.f, 0.f, 0.f, 1.f};

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = extent;
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clear_values.size());
        renderPassInfo.pClearValues = clear_values.data();

        for (uint32_t i = 0; i < this->m_buffers.size(); ++i) {
            if ( VK_SUCCESS != vkBeginCommandBuffer(this->m_buffers[i], &beginInfo) ) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }
            {
                renderPassInfo.framebuffer = swapChainFbufs[i];

                vkCmdBeginRenderPass(this->m_buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                {
                    vkCmdBindPipeline(this->m_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_deferred);

                    for (const auto& model : models) {
                        for (uint32_t unit_index = 0; unit_index < model.render_units().size(); ++unit_index) {
                            const auto& render_unit = model.render_units().at(unit_index);

                            VkBuffer vertBuffers[] = {render_unit.m_mesh.vertices.getBuf()};
                            VkDeviceSize offsets[] = {0};
                            vkCmdBindVertexBuffers(this->m_buffers[i], 0, 1, vertBuffers, offsets);
                            vkCmdBindIndexBuffer(this->m_buffers[i], render_unit.m_mesh.indices.getBuf(), 0, VK_INDEX_TYPE_UINT32);

                            for (uint32_t inst_index = 0; inst_index < model.instances().size(); ++inst_index) {
                                const auto& inst = model.instances().at(inst_index);

                                vkCmdBindDescriptorSets(
                                    this->m_buffers[i],
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipelayout_deferred,
                                    0, 1, &model.desc_set(i, inst_index, unit_index).get(), 0, nullptr
                                );

                                vkCmdDrawIndexed(this->m_buffers[i], render_unit.m_mesh.indices.size(), 1, 0, 0, 0);
                            }
                        }
                    }
                }
                {
                    vkCmdNextSubpass(this->m_buffers[i], VK_SUBPASS_CONTENTS_INLINE);
                    vkCmdBindPipeline(this->m_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_composition);
                    vkCmdBindDescriptorSets(
                        this->m_buffers[i],
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipelayout_composition,
                        0, 1, &descset_composition.front()[i], 0, nullptr
                    );
                    vkCmdDraw(this->m_buffers[i], 6, 1, 0, 0);
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
