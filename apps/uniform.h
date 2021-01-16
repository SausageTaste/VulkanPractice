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
    class UniformBufferArray {

    private:
        std::vector<VkBuffer> m_buffers;
        std::vector<VkDeviceMemory> m_memories;

    public:
        void init(const uint32_t array_size, const VkDevice logi_device, const VkPhysicalDevice phys_device) {
            this->destroy(logi_device);

            for (uint32_t i = 0; i < array_size; ++i) {
                this->m_buffers.emplace_back();
                this->m_memories.emplace_back();

                dal::createBuffer(
                    this->data_size(),
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    this->m_buffers.back(), this->m_memories.back(),
                    logi_device, phys_device
                );
            }
        }

        void destroy(const VkDevice logi_device) {
            for (auto& x : this->m_buffers) {
                if (VK_NULL_HANDLE != x) {
                    vkDestroyBuffer(logi_device, x, nullptr);
                    x = VK_NULL_HANDLE;
                }
            }
            this->m_buffers.clear();

            for (auto& x : this->m_memories) {
                if (VK_NULL_HANDLE != x) {
                    vkFreeMemory(logi_device, x, nullptr);
                    x = VK_NULL_HANDLE;
                }
            }
            this->m_memories.clear();
        }

        constexpr uint32_t data_size() const {
            return sizeof(_DataStruct);
        }
        uint32_t array_size() const {
            assert(this->m_buffers.size() == this->m_memories.size());
            return this->m_buffers.size();
        }
        auto buffer_at(const uint32_t index) const {
            return this->m_buffers.at(index);
        }

        void copy_to_buffer(const size_t index, const _DataStruct& data, const VkDevice logi_device) {
            const auto memory = this->m_memories.at(index);
            void* dst_ptr = nullptr;
            vkMapMemory(logi_device, memory, 0, this->data_size(), 0, &dst_ptr);
            memcpy(dst_ptr, &data, this->data_size());
            vkUnmapMemory(logi_device, memory);
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
            const UniformBufferArray<U_PerFrame_InDeferred>& per_frame_in_deferred,
            const UniformBufferArray<U_Material_InDeferred>& material_buffer,
            const VkImageView textureImageView,
            const VkSampler textureSampler,
            const VkDevice logi_device
        );
        void record_composition(
            const size_t swapchainImagesSize,
            const UniformBufferArray<U_PerFrame_InComposition>& u_per_frame,
            const VkDescriptorSetLayout descriptorSetLayout,
            const std::vector<VkImageView>& attachment_views,
            const std::vector<VkImageView>& dlight_shadow_map_view,
            const VkSampler dlight_shadow_map_sampler,
            const VkDevice logiDevice
        );
        void record_shadow(const VkDevice logi_device);

        auto& at(const uint32_t swapchain_index) const {
            return this->m_handles.at(swapchain_index);
        }
        auto& vector() {
            return this->m_handles;
        }
        auto& vector() const {
            return this->m_handles;
        }

    };


    class DescriptorPool {

    private:
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        std::vector<DescriptorSet> m_descset_composition;
        DescriptorSet m_descset_shadow;

    public:
        void initPool(VkDevice logiDevice, size_t swapchainImagesSize);
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

        auto& pool() const {
            return this->descriptorPool;
        }
        std::vector<std::vector<VkDescriptorSet>> descset_composition() const;
        auto& descset_shadow() const {
            return this->m_descset_shadow.vector();
        }

    };

}
