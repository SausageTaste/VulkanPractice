#pragma once

#include <array>
#include <vector>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>


namespace dal {

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;

        static VkVertexInputBindingDescription getBindingDesc();
        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
    };


    class VertexBuffer {

    private:
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory buffer_mem = VK_NULL_HANDLE;
        uint32_t vertSize = 0;

    public:
        void init(const std::vector<Vertex>& vertices, const VkDevice logiDevice,
            const VkPhysicalDevice physDevice, VkCommandPool cmdPool, VkQueue graphicsQueue);
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
        void init(const std::vector<uint16_t>& indices, const VkDevice logiDevice,
            const VkPhysicalDevice physDevice, VkCommandPool cmdPool, VkQueue graphicsQueue);
        void destroy(const VkDevice device);

        auto getBuf() const {
            assert(VK_NULL_HANDLE != this->indexBuffer);
            return this->indexBuffer;
        }
        uint32_t size() const {
            return this->arr_size;
        }

    };

}
