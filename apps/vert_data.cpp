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


    const std::vector<Vertex>& getDemoVertices() {
        static const std::vector<Vertex> VERTICES = {
            {{ 0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
            {{ 0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}
        };

        return VERTICES;
    }

}


namespace dal {

    void VertexBuffer::init(const std::vector<Vertex>& vertices, const VkDevice logiDevice, const VkPhysicalDevice physDevice) {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        ::createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            this->buffer,
            this->buffer_mem,
            logiDevice,
            physDevice
        );

        void* data;
        vkMapMemory(logiDevice, this->buffer_mem, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(logiDevice, this->buffer_mem);

        this->vertSize = static_cast<uint32_t>(vertices.size());
    }

    void VertexBuffer::destroy(const VkDevice device) {
        vkDestroyBuffer(device, this->buffer, nullptr);
        this->buffer = VK_NULL_HANDLE;

        vkFreeMemory(device, this->buffer_mem, nullptr);
        this->buffer_mem = VK_NULL_HANDLE;

        this->vertSize = 0;
    }

}
