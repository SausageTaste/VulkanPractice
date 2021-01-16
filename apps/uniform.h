#pragma once

#include <tuple>
#include <vector>
#include <cassert>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "fbufmanager.h"
#include "util_vulkan.h"


namespace dal {

    struct U_PerFrame_InDeferred {
        glm::mat4 view, proj;
    };

    struct U_Material_InDeferred {
        float m_roughness = 0.5;
        float m_metallic = 0;
    };

    struct U_PerFrame_InComposition {
        glm::vec4 m_view_pos{ 0 };

        glm::vec4 m_num_of_plight_dlight_slight{ 0 };

        glm::vec4 m_plight_color[5]{};
        glm::vec4 m_plight_pos[5]{};

        glm::vec4 m_dlight_color[3]{};
        glm::vec4 m_dlight_direc[3]{};
        glm::mat4 m_dlight_mat[3]{};

        glm::vec4 m_slight_pos[5]{};
        glm::vec4 m_slight_direc[5]{};
        glm::vec4 m_slight_color[5]{};
        glm::vec4 m_slight_fade_start_end[5]{};
    };

}


namespace dal {

    template <typename _DataStruct>
    class UniformBuffer {

    private:
        VkBuffer m_buffer = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;

    public:
        void init(const VkDevice logi_device, const VkPhysicalDevice phys_device) {
            this->destroy(logi_device);

            dal::createBuffer(
                this->data_size(),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                this->m_buffer, this->m_memory,
                logi_device, phys_device
            );
        }

        void destroy(const VkDevice logi_device) {
            if (VK_NULL_HANDLE != this->m_buffer) {
                vkDestroyBuffer(logi_device, this->m_buffer, nullptr);
                this->m_buffer = VK_NULL_HANDLE;
            }

            if (VK_NULL_HANDLE != this->m_memory) {
                vkFreeMemory(logi_device, this->m_memory, nullptr);
                this->m_memory = VK_NULL_HANDLE;
            }
        }

        constexpr uint32_t data_size() const {
            return sizeof(_DataStruct);
        }
        auto buffer() const {
            return this->m_buffer;
        }

        void copy_to_buffer(const _DataStruct& data, const VkDevice logi_device) {
            void* dst_ptr = nullptr;
            vkMapMemory(logi_device, this->m_memory, 0, this->data_size(), 0, &dst_ptr);
            memcpy(dst_ptr, &data, this->data_size());
            vkUnmapMemory(logi_device, this->m_memory);
        }

    };


    template <typename _DataStruct>
    class UniformBufferArray {

    private:
        std::vector<UniformBuffer<_DataStruct>> m_buffers;

    public:
        void init(const uint32_t array_size, const VkDevice logi_device, const VkPhysicalDevice phys_device) {
            for (uint32_t i = 0; i < array_size; ++i) {
                this->m_buffers.emplace_back().init(logi_device, phys_device);
            }
        }

        void destroy(const VkDevice logi_device) {
            for (auto& x : this->m_buffers) {
                x.destroy(logi_device);
            }
            this->m_buffers.clear();
        }

        constexpr uint32_t data_size() const {
            return sizeof(_DataStruct);
        }
        uint32_t array_size() const {
            return this->m_buffers.size();
        }
        auto& buffer_at(const uint32_t index) const {
            return this->m_buffers.at(index);
        }

        void copy_to_buffer(const size_t index, const _DataStruct& data, const VkDevice logi_device) {
            this->m_buffers.at(index).copy_to_buffer(data, logi_device);
        }

    };


    class DescriptorSetLayout {

    private:
        VkDescriptorSetLayout m_layout_deferred = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_layout_composition = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_layout_shadow = VK_NULL_HANDLE;

    public:
        void init(const VkDevice logiDevice);
        void destroy(const VkDevice logiDevice);

        auto& layout_deferred() const {
            return this->m_layout_deferred;
        }
        auto& layout_composition() const {
            return this->m_layout_composition;
        }
        auto& layout_shadow() const {
            return this->m_layout_shadow;
        }

    };


    class DescSet {

    private:
        VkDescriptorSet m_handle = VK_NULL_HANDLE;

    public:
        void set(const VkDescriptorSet desc_set) {
            this->m_handle = desc_set;
        }

        auto& get() const {
            return this->m_handle;
        }

        void record_deferred(
            const UniformBuffer<U_PerFrame_InDeferred>& ubuf_per_frame_in_deferred,
            const UniformBuffer<U_Material_InDeferred>& ubuf_material,
            const VkImageView textureImageView,
            const VkSampler textureSampler,
            const VkDevice logi_device
        );
        void record_composition(
            const size_t swapchainImagesSize,
            const UniformBuffer<U_PerFrame_InComposition> ubuf_per_frame,
            const VkDescriptorSetLayout descriptorSetLayout,
            const std::vector<VkImageView>& attachment_views,
            const std::vector<VkImageView>& dlight_shadow_map_view,
            const VkSampler dlight_shadow_map_sampler,
            const VkDevice logiDevice
        );
        void record_shadow(
            const VkDevice logi_device
        );

    };


    class DescPool {

    private:
        VkDescriptorPool m_pool = VK_NULL_HANDLE;

    public:
        void init(
            const uint32_t uniform_buf_count,
            const uint32_t image_sampler_count,
            const uint32_t input_attachment_count,
            const uint32_t desc_set_count,
            const VkDevice logi_device
        );
        void destroy(const VkDevice logi_device);
        void reset(const VkDevice logi_device);

        DescSet allocate(const VkDescriptorSetLayout layout, const VkDevice logi_device);
        std::vector<DescSet> allocate(const uint32_t count, const VkDescriptorSetLayout layout, const VkDevice logi_device);

        auto get() const {
            return this->m_pool;
        }

    };


    class DescriptorSetManager {

    private:
        DescPool m_pool;

        std::vector<std::vector<DescSet>> m_descset_composition;
        std::vector<DescSet> m_descset_shadow;

    public:
        void init(const uint32_t swapchain_count, const VkDevice logi_device);
        void addSets_composition(
            const VkDevice logiDevice,
            const size_t swapchainImagesSize,
            const VkDescriptorSetLayout descriptorSetLayout,
            const UniformBufferArray<U_PerFrame_InComposition>& ubuf_per_frame,
            const std::vector<VkImageView>& attachment_views,
            const std::vector<VkImageView>& dlight_shadow_map_view,
            const VkSampler dlight_shadow_map_sampler
        );
        void init_descset_shadow(
            const uint32_t swapchain_count,
            const VkDescriptorSetLayout descset_layout,
            const VkDevice logi_device
        );
        void destroy(VkDevice logiDevice);

        auto& pool() {
            return this->m_pool;
        }

        std::vector<std::vector<VkDescriptorSet>> descset_composition() const;
        std::vector<VkDescriptorSet> descset_shadow() const;

    };

}
