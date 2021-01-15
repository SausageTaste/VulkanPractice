#pragma once

#include <tuple>
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

    struct U_Material {
        float m_roughness = 0.5;
        float m_metallic = 0;
    };


    std::pair<VkBuffer, VkDeviceMemory> _init_uniform_buffer(const void* const data, const VkDeviceSize data_size, const VkDevice logi_device, const VkPhysicalDevice phys_device);
    void _destroy_buffer_memory(const VkBuffer buffer, const VkDeviceMemory memory, const VkDevice logi_device);

    template <typename _UniformStruct>
    class UniformBufferConst {

    private:
        VkBuffer m_buffer = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;

    public:
        void init(const _UniformStruct& data, const VkDevice logi_device, const VkPhysicalDevice phys_device) {
            this->destroy(logi_device);
            std::tie(this->m_buffer, this->m_memory) = dal::_init_uniform_buffer(
                reinterpret_cast<const void*>(&data),
                this->data_size(),
                logi_device,
                phys_device
            );
        }
        void destroy(const VkDevice logi_device) {
            _destroy_buffer_memory(this->m_buffer, this->m_memory, logi_device);
            this->m_buffer = VK_NULL_HANDLE;
            this->m_memory = VK_NULL_HANDLE;
        }

        auto& buffer() const {
            assert(VK_NULL_HANDLE != this->m_buffer);
            return this->m_buffer;
        }
        auto data_size() const {
            return sizeof(_UniformStruct);
        }

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

    class UniformBuffer_PerFrame {

    private:
        std::vector<VkBuffer> m_buffers;
        std::vector<VkDeviceMemory> m_memories;
        VkDeviceSize m_data_size = 0;

    public:
        void init(const VkDeviceSize data_size, const uint32_t swapchain_count, const VkDevice logi_device, const VkPhysicalDevice phys_device);
        void destroy(const VkDevice logiDevice);

        void copy_to_memory(const uint32_t index, const void* const data, const VkDeviceSize data_size, const VkDevice logi_device) const;

        auto buffer(const uint32_t index) const {
            return this->m_buffers.at(index);
        }
        auto data_size() const {
            return this->m_data_size;
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
            const std::vector<VkBuffer>& uniformBuffers,
            const UniformBufferConst<U_Material>& m_material_buffer,
            const VkImageView textureImageView,
            const VkSampler textureSampler,
            const VkDevice logi_device
        );
        void record_composition(
            const size_t swapchainImagesSize,
            const UniformBuffer_PerFrame& u_per_frame,
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
            const UniformBuffer_PerFrame& ubuf_per_frame,
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


    class UniformBuffers {

    private:
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;

    public:
        void init(VkDevice logiDevice, VkPhysicalDevice physDevice, size_t swapchainImagesSize);
        void destroy(const VkDevice logiDevice);

        void update(const glm::mat4& proj_mat, const glm::mat4& view_mat, const uint32_t imageIndex, const VkDevice logiDevice);
        void copy_to_memory(const VkDevice logiDevice, const UniformBufferObject& ubo, const uint32_t index) const;

        auto& buffers() const {
            assert(0 != this->uniformBuffers.size());
            return this->uniformBuffers;
        }

    };

}
