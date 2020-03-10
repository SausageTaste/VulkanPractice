#include "vkommand.h"

#include <stdexcept>

#include "util_vulkan.h"


namespace {

    VkCommandPool createCommandPool(VkPhysicalDevice PhysDevice, VkDevice logiDevice, VkSurfaceKHR surface) {
        dal::QueueFamilyIndices queueFamilyIndices = dal::findQueueFamilies(PhysDevice, surface);

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.m_graphicsFamily.value();
        poolInfo.flags = 0; // Optional

        VkCommandPool commandPool = VK_NULL_HANDLE;
        if ( vkCreateCommandPool(logiDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS ) {
            throw std::runtime_error("failed to create command pool!");
        }

        return commandPool;
    }
}


namespace dal {

    void CommandPool::init(VkPhysicalDevice physDevice, VkDevice logiDevice, VkSurfaceKHR surface, const unsigned cmdBufSize) {
        this->m_pool = createCommandPool(physDevice, logiDevice, surface);

        // Create command buffers
        {
            this->m_buffers.resize(cmdBufSize);

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

                if ( vkBeginCommandBuffer(this->m_buffers[i], &beginInfo) != VK_SUCCESS ) {
                    throw std::runtime_error("failed to begin recording command buffer!");
                }
            }
        }
    }

    void CommandPool::destroy(VkDevice logiDevice) {
        vkDestroyCommandPool(logiDevice, this->m_pool, nullptr);
        this->m_pool = VK_NULL_HANDLE;
    }

}
