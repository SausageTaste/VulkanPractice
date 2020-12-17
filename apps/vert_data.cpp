#include "vert_data.h"

#include <stdexcept>


namespace {

    uint32_t findMemType(const uint32_t typeFilter, const VkMemoryPropertyFlags props, const VkPhysicalDevice physDevice) {
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(physDevice, &memProps);

        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
            if (typeFilter & (1 << i) && (memProps.memoryTypes[i].propertyFlags & props) == props) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
        VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDevice logiDevice, VkPhysicalDevice physDevice)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(logiDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(logiDevice, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = ::findMemType(
            memRequirements.memoryTypeBits, properties, physDevice
        );

        if (vkAllocateMemory(logiDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(logiDevice, buffer, bufferMemory, 0);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,
        VkDevice logiDevice, VkCommandPool cmdPool, VkQueue graphicsQueue) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = cmdPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(logiDevice, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        {
            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = 0; // Optional
            copyRegion.dstOffset = 0; // Optional
            copyRegion.size = size;
            vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        }
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(logiDevice, cmdPool, 1, &commandBuffer);
    }

}


namespace dal {

    VkVertexInputBindingDescription Vertex::getBindingDesc() {
        VkVertexInputBindingDescription result;

        result.binding = 0;
        result.stride = sizeof(dal::Vertex);
        result.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return result;
    }

    std::array<VkVertexInputAttributeDescription, 2> Vertex::getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> result;

        result[0].binding = 0;
        result[0].location = 0;
        result[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        result[0].offset = offsetof(Vertex, pos);

        result[1].binding = 0;
        result[1].location = 1;
        result[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        result[1].offset = offsetof(Vertex, color);

        return result;
    }

}


namespace dal {

    void VertexBuffer::init(const std::vector<Vertex>& vertices, const VkDevice logiDevice,
        const VkPhysicalDevice physDevice, VkCommandPool cmdPool, VkQueue graphicsQueue)
    {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
        ::createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory,
            logiDevice,
            physDevice
        );

        void* data;
        vkMapMemory(logiDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(logiDevice, stagingBufferMemory);

        ::createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            this->buffer,
            this->buffer_mem,
            logiDevice,
            physDevice
        );

        ::copyBuffer(stagingBuffer, this->buffer, bufferSize, logiDevice, cmdPool, graphicsQueue);
        this->vertSize = static_cast<uint32_t>(vertices.size());

        vkDestroyBuffer(logiDevice, stagingBuffer, nullptr);
        vkFreeMemory(logiDevice, stagingBufferMemory, nullptr);
    }

    void VertexBuffer::destroy(const VkDevice device) {
        vkDestroyBuffer(device, this->buffer, nullptr);
        this->buffer = VK_NULL_HANDLE;

        vkFreeMemory(device, this->buffer_mem, nullptr);
        this->buffer_mem = VK_NULL_HANDLE;

        this->vertSize = 0;
    }

}


namespace dal {

    void IndexBuffer::init(const std::vector<uint16_t>& indices, const VkDevice logiDevice,
            const VkPhysicalDevice physDevice, VkCommandPool cmdPool, VkQueue graphicsQueue)
    {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        ::createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory,
            logiDevice,
            physDevice
        );

        void* data;
        vkMapMemory(logiDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t) bufferSize);
        vkUnmapMemory(logiDevice, stagingBufferMemory);

        ::createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            indexBuffer,
            indexBufferMemory,
            logiDevice,
            physDevice
        );

        ::copyBuffer(stagingBuffer, indexBuffer, bufferSize, logiDevice, cmdPool, graphicsQueue);
        this->arr_size = indices.size();

        vkDestroyBuffer(logiDevice, stagingBuffer, nullptr);
        vkFreeMemory(logiDevice, stagingBufferMemory, nullptr);
    }

    void IndexBuffer::destroy(const VkDevice device) {
        vkDestroyBuffer(device, this->indexBuffer, nullptr);
        this->indexBuffer = VK_NULL_HANDLE;

        vkFreeMemory(device, this->indexBufferMemory, nullptr);
        this->indexBufferMemory = VK_NULL_HANDLE;

        this->arr_size = 0;
    }

}
