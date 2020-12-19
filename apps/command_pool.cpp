#include "command_pool.h"

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

    VkCommandBuffer CommandPool::beginSingleTimeCmd(const VkDevice logiDevice) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = this->pool();
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(logiDevice, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void CommandPool::endSingleTimeCmd(VkCommandBuffer cmdBuffer, VkDevice logiDevice, VkQueue graphicsQueue) {
        vkEndCommandBuffer(cmdBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(logiDevice, this->pool(), 1, &cmdBuffer);
    }

}
