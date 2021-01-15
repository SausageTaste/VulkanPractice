#pragma once

#include <cassert>
#include <vector>

#include <vulkan/vulkan.h>

#include "model_render.h"


namespace dal {

    class CommandBuffers {

    private:
        std::vector<VkCommandBuffer> m_buffers;
        std::vector<VkCommandBuffer> m_shadow_map_cmd;

    public:
        void init(
            const VkDevice logiDevice,
            const size_t swapchain_count,
            const VkCommandPool cmdPool
        );
        void destroy(const VkDevice logiDevice, const VkCommandPool cmdPool);

        void record(
            const VkRenderPass renderPass,
            const VkPipeline pipeline_deferred,
            const VkPipeline pipeline_composition,
            const VkPipelineLayout pipelayout_deferred,
            const VkPipelineLayout pipelayout_composition,
            const VkExtent2D& extent,
            const std::vector<VkFramebuffer>& swapChainFbufs,
            const std::vector<std::vector<VkDescriptorSet>>& descset_composition,
            const std::vector<ModelVK>& models
        );
        void record_shadow(
            const VkRenderPass renderpass,
            const VkPipeline pipeline,
            const VkPipelineLayout pipelayout,
            const VkExtent2D& extent,
            const VkFramebuffer fbuf,
            const VkDescriptorSet descset_shadow,
            const std::vector<ModelVK>& models
        );

        auto& buffers(void) const {
            assert(0 != this->m_buffers.size());
            return this->m_buffers;
        }

    };

}
