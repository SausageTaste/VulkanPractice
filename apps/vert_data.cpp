#include "vert_data.h"

#include <stdexcept>

#include "util_vulkan.h"


namespace {

    void copyBuffer(
        VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,
        VkDevice logiDevice, dal::CommandPool& cmdPool, VkQueue graphicsQueue
    ) {
        const auto commandBuffer = cmdPool.beginSingleTimeCmd(logiDevice);
        {
            VkBufferCopy copyRegion{};
            copyRegion.size = size;
            vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        }
        cmdPool.endSingleTimeCmd(commandBuffer, logiDevice, graphicsQueue);
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

    std::array<VkVertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> result;

        result[0].binding = 0;
        result[0].location = 0;
        result[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        result[0].offset = offsetof(Vertex, pos);

        result[1].binding = 0;
        result[1].location = 1;
        result[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        result[1].offset = offsetof(Vertex, color);

        result[2].binding = 0;
        result[2].location = 2;
        result[2].format = VK_FORMAT_R32G32_SFLOAT;
        result[2].offset = offsetof(Vertex, texCoord);

        return result;
    }

}


namespace dal {

    void VertexBuffer::init(const std::vector<Vertex>& vertices, const VkDevice logiDevice,
        const VkPhysicalDevice physDevice, dal::CommandPool& cmdPool, VkQueue graphicsQueue)
    {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
        dal::createBuffer(
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

        dal::createBuffer(
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
            const VkPhysicalDevice physDevice, dal::CommandPool& cmdPool, VkQueue graphicsQueue)
    {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        dal::createBuffer(
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

        dal::createBuffer(
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
        if (VK_NULL_HANDLE != this->indexBuffer) {
            vkDestroyBuffer(device, this->indexBuffer, nullptr);
            this->indexBuffer = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != this->indexBufferMemory) {
            vkFreeMemory(device, this->indexBufferMemory, nullptr);
            this->indexBufferMemory = VK_NULL_HANDLE;
        }

        this->arr_size = 0;
    }

}
