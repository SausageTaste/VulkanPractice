#pragma once

#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>


namespace dal {

    struct UniformBufferObject {
        glm::mat4 model, view, proj;
    };


    class DescriptorSetLayout {

    private:
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

    public:
        void init(const VkDevice logiDevice);
        void destroy(const VkDevice logiDevice);

        auto& get() const {
            return this->descriptorSetLayout;
        }

    };


    class DescriptorPool {

    private:
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        std::vector<std::vector<VkDescriptorSet>> descriptorSetsList;

    public:
        void initPool(VkDevice logiDevice, size_t swapchainImagesSize);
        void addSets(
            VkDevice logiDevice, size_t swapchainImagesSize, VkDescriptorSetLayout descriptorSetLayout,
            const std::vector<VkBuffer>& uniformBuffers, VkImageView textureImageView, VkSampler textureSampler
        );
        void destroy(VkDevice logiDevice);

        size_t size() const {
            return this->descriptorSetsList.size();
        }
        auto& descSetsList() const {
            return this->descriptorSetsList;
        }

    };


    class UniformBuffers {

    private:
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;

    public:
        void init(VkDevice logiDevice, VkPhysicalDevice physDevice, size_t swapchainImagesSize);
        void destroy(const VkDevice logiDevice);

        void update(const uint32_t imageIndex, const VkExtent2D swapchainExtent, const VkDevice logiDevice);

        auto& buffers() const {
            assert(0 != this->uniformBuffers.size());
            return this->uniformBuffers;
        }

    };

}
