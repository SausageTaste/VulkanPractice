#pragma once

#include <cassert>
#include <vector>

#include <vulkan/vulkan.h>

#include "vert_data.h"


namespace dal {

    class CommandPool {

    private:
        VkCommandPool m_pool = VK_NULL_HANDLE;

    public:
        void init(VkPhysicalDevice physDevice, VkDevice logiDevice, VkSurfaceKHR surface);
        void destroy(const VkDevice logiDevice);

        auto& pool() const {
            assert(VK_NULL_HANDLE != this->m_pool);
            return this->m_pool;
        }
    };


    class CommandBuffers {

    private:
        std::vector<VkCommandBuffer> m_buffers;

    public:
        void init(VkDevice logiDevice, const std::vector<VkFramebuffer>& swapChainFbufs, VkCommandPool cmdPool);
        void destroy(const VkDevice logiDevice, const VkCommandPool cmdPool);

        void record(
            VkRenderPass renderPass, VkPipeline graphicsPipeline, const VkExtent2D& extent, const std::vector<VkFramebuffer>& swapChainFbufs,
            VkPipelineLayout pipelineLayout, const std::vector<VkDescriptorSet>& descriptorSets, const std::vector<MeshBuffer>& meshes
        );

        auto& buffers(void) const {
            assert(0 != this->m_buffers.size());
            return this->m_buffers;
        }

    };

}
