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
        VkBufferCreateInfo bufInfo{};
        bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size = sizeof(vertices[0]) * vertices.size();
        bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (VK_SUCCESS != vkCreateBuffer(logiDevice, &bufInfo, nullptr, &buffer)) {
            throw std::runtime_error("failed to create vertex buffer!");
        }
        if (VK_NULL_HANDLE == this->buffer) {
            throw std::runtime_error("created vertex buffer but it is still NULL!");
        }

        VkMemoryRequirements memRequirements{};
        vkGetBufferMemoryRequirements(logiDevice, this->buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = ::findMemType(
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            physDevice
        );

        if (VK_SUCCESS != vkAllocateMemory(logiDevice, &allocInfo, nullptr, &this->buffer_mem)) {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        vkBindBufferMemory(logiDevice, this->buffer, this->buffer_mem, 0);

        void* data;
        vkMapMemory(logiDevice, this->buffer_mem, 0, bufInfo.size, 0, &data);
        memcpy(data, vertices.data(), static_cast<size_t>(bufInfo.size));
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
