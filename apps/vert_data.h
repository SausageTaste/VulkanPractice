#pragma once

#include <array>
#include <vector>

#include <vulkan/vulkan.h>

#include "command_pool.h"
#include "model_data.h"


namespace dal {

    VkVertexInputBindingDescription getBindingDesc();
    std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();


    class VertexBuffer {

    private:
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory buffer_mem = VK_NULL_HANDLE;
        uint32_t vertSize = 0;

    public:
        VertexBuffer() = default;
        VertexBuffer(VertexBuffer&&) = default;
        VertexBuffer& operator=(VertexBuffer&&) = default;

        VertexBuffer(const VertexBuffer&) = delete;
        VertexBuffer& operator=(const VertexBuffer&) = delete;

    public:
        void init(const std::vector<Vertex>& vertices, const VkDevice logiDevice,
            const VkPhysicalDevice physDevice, dal::CommandPool& cmdPool, VkQueue graphicsQueue);
        void destroy(const VkDevice device);

        auto getBuf() const {
            assert(VK_NULL_HANDLE != this->buffer);
            return this->buffer;
        }
        uint32_t size() const {
            return this->vertSize;
        }

    };


    class IndexBuffer {

    private:
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
        uint32_t arr_size = 0;

    public:
        IndexBuffer() = default;
        IndexBuffer(IndexBuffer&&) = default;
        IndexBuffer& operator=(IndexBuffer&&) = default;

        IndexBuffer(const IndexBuffer&) = delete;
        IndexBuffer& operator=(const IndexBuffer&) = delete;

    public:
        void init(const std::vector<uint32_t>& indices, const VkDevice logiDevice,
            const VkPhysicalDevice physDevice, dal::CommandPool& cmdPool, VkQueue graphicsQueue);
        void destroy(const VkDevice device);

        auto getBuf() const {
            assert(VK_NULL_HANDLE != this->indexBuffer);
            return this->indexBuffer;
        }
        uint32_t size() const {
            return this->arr_size;
        }

    };


    struct MeshBuffer {
        VertexBuffer vertices;
        IndexBuffer indices;

        MeshBuffer() = default;
        MeshBuffer(MeshBuffer&&) = default;
        MeshBuffer& operator=(MeshBuffer&&) = default;

        MeshBuffer(const MeshBuffer&) = delete;
        MeshBuffer& operator=(const MeshBuffer&) = delete;
    };

}
