#pragma once

#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "fbufmanager.h"


namespace dal {

    struct UniformBufferObject {
        glm::mat4 view, proj;
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


    class DescriptorSet {

    private:
        std::vector<VkDescriptorSet> m_handles;

    public:
        void destroy(const VkDescriptorPool pool, const VkDevice logi_device);

        void init(
            const uint32_t swapchain_count,
            const VkDescriptorSetLayout descriptor_set_layout,
            const VkDescriptorPool pool,
            const VkDevice logi_device
        );

        void record_deferred(
            const std::vector<VkBuffer>& uniformBuffers,
            const VkImageView textureImageView,
            const VkSampler textureSampler,
            const VkDevice logi_device
        );
        void record_composition(
            const size_t swapchainImagesSize,
            const VkDescriptorSetLayout descriptorSetLayout,
            const std::vector<VkImageView>& attachment_views,
            const VkDevice logiDevice
        );

        auto& at(const uint32_t swapchain_index) const {
            return this->m_handles.at(swapchain_index);
        }
        auto& vector() const {
            return this->m_handles;
        }

    };


    class DescriptorPool {

    private:
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        std::vector<DescriptorSet> m_descset_deferred, m_descset_composition;

    public:
        void initPool(VkDevice logiDevice, size_t swapchainImagesSize);
        void addSets_composition(
            const VkDevice logiDevice,
            const size_t swapchainImagesSize,
            const VkDescriptorSetLayout descriptorSetLayout,
            const std::vector<VkImageView>& attachment_views
        );
        void destroy(VkDevice logiDevice);

        auto& pool() const {
            return this->descriptorPool;
        }
        std::vector<std::vector<VkDescriptorSet>> descset_composition() const;

    };


    class UniformBuffers {

    private:
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;

    public:
        void init(VkDevice logiDevice, VkPhysicalDevice physDevice, size_t swapchainImagesSize);
        void destroy(const VkDevice logiDevice);

        void update(const uint32_t imageIndex, const VkExtent2D swapchainExtent, const VkDevice logiDevice);
        void copy_to_memory(const VkDevice logiDevice, const UniformBufferObject& ubo, const uint32_t index) const;

        auto& buffers() const {
            assert(0 != this->uniformBuffers.size());
            return this->uniformBuffers;
        }

    };

}
