#pragma once

#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "fbufmanager.h"


namespace dal {

    struct UniformBufferObject {
        glm::mat4 model, view, proj;
    };


    class DescriptorSetLayout {

    private:
        VkDescriptorSetLayout m_layout_deferred = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_layout_composition = VK_NULL_HANDLE;

    public:
        void init(const VkDevice logiDevice);
        void destroy(const VkDevice logiDevice);

        auto& layout_deferred() const {
            return this->m_layout_deferred;
        }
        auto& layout_composition() const {
            return this->m_layout_composition;
        }

    };


    class DescriptorPool {

    private:
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    public:
        std::vector<std::vector<VkDescriptorSet>> m_descset_deferred, m_descset_composition;

    public:
        void initPool(VkDevice logiDevice, size_t swapchainImagesSize);
        void addSets_deferred(
            VkDevice logiDevice, size_t swapchainImagesSize, VkDescriptorSetLayout descriptorSetLayout,
            const std::vector<VkBuffer>& uniformBuffers, VkImageView textureImageView, VkSampler textureSampler
        );
        void addSets_composition(
            const VkDevice logiDevice,
            const size_t swapchainImagesSize,
            const VkDescriptorSetLayout descriptorSetLayout,
            const std::vector<VkImageView>& attachment_views
        );
        void destroy(VkDevice logiDevice);

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
