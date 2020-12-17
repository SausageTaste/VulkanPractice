#pragma once

#include <cassert>
#include <vector>

#include <vulkan/vulkan.h>


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
        void init(
            VkDevice logiDevice, VkRenderPass renderPass, VkPipeline graphicsPipeline,
            const VkExtent2D& extent, const std::vector<VkFramebuffer>& swapChainFbufs, VkCommandPool cmdPool,
            const VkBuffer vertBuf, const uint32_t vertSize, const VkBuffer indexBuffer, const uint32_t indexSize,
            VkPipelineLayout pipelineLayout, const std::vector<VkDescriptorSet>& descriptorSets
        );
        void destroy(const VkDevice logiDevice, const VkCommandPool cmdPool);

        auto& buffers(void) const {
            assert(0 != this->m_buffers.size());
            return this->m_buffers;
        }

    };

}
