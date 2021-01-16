#include "uniform.h"

#include <array>
#include <chrono>
#include <stdexcept>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>


namespace {

    VkDescriptorSetLayout create_layout_deferred(const VkDevice logiDevice) {
        std::array<VkDescriptorSetLayoutBinding, 3> bindings{};

        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bindings[0].pImmutableSamplers = nullptr; // Optional

        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[1].pImmutableSamplers = nullptr;

        bindings[2].binding = 2;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[2].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout result = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logiDevice, &layoutInfo, nullptr, &result)) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        return result;
    }

    VkDescriptorSetLayout create_layout_composition(const VkDevice logiDevice) {
        std::array<VkDescriptorSetLayoutBinding, 7> bindings{};

        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[2].binding = 2;
        bindings[2].descriptorCount = 1;
        bindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[3].binding = 3;
        bindings[3].descriptorCount = 1;
        bindings[3].descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[4].binding = 4;
        bindings[4].descriptorCount = 1;
        bindings[4].descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[5].binding = 5;
        bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[5].descriptorCount = 1;
        bindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[5].pImmutableSamplers = nullptr;

        bindings[6].binding = 6;
        bindings[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[6].descriptorCount = 3;
        bindings[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout result = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logiDevice, &layoutInfo, nullptr, &result)) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        return result;
    }

    VkDescriptorSetLayout create_layout_shadow(const VkDevice logiDevice) {
        std::array<VkDescriptorSetLayoutBinding, 0> bindings{};

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout result = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logiDevice, &layoutInfo, nullptr, &result)) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        return result;
    }

}

namespace dal {

    void DescriptorSetLayout::init(const VkDevice logiDevice) {
        this->destroy(logiDevice);

        this->m_layout_deferred = ::create_layout_deferred(logiDevice);
        this->m_layout_composition = ::create_layout_composition(logiDevice);
        this->m_layout_shadow = ::create_layout_shadow(logiDevice);
    }

    void DescriptorSetLayout::destroy(const VkDevice logiDevice) {
        if (VK_NULL_HANDLE != this->m_layout_deferred) {
            vkDestroyDescriptorSetLayout(logiDevice, this->m_layout_deferred, nullptr);
            this->m_layout_deferred = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != this->m_layout_composition) {
            vkDestroyDescriptorSetLayout(logiDevice, this->m_layout_composition, nullptr);
            this->m_layout_composition = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != this->m_layout_shadow) {
            vkDestroyDescriptorSetLayout(logiDevice, this->m_layout_shadow, nullptr);
            this->m_layout_shadow = VK_NULL_HANDLE;
        }
    }

}


namespace dal {

    void DescriptorSet::destroy(const VkDescriptorPool pool, const VkDevice logi_device) {
        if (!this->m_handles.empty()) {
            vkFreeDescriptorSets(logi_device, pool, this->m_handles.size(), this->m_handles.data());
            this->m_handles.clear();
        }
    }

    void DescriptorSet::init(
        const uint32_t swapchain_count,
        const VkDescriptorSetLayout descriptor_set_layout,
        const VkDescriptorPool pool,
        const VkDevice logi_device
    ) {
        std::vector<VkDescriptorSetLayout> layouts(swapchain_count, descriptor_set_layout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = swapchain_count;
        allocInfo.pSetLayouts = layouts.data();

        this->m_handles.resize(swapchain_count);

        if (vkAllocateDescriptorSets(logi_device, &allocInfo, this->m_handles.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

    }

    void DescriptorSet::record_deferred(
        const UniformBufferArray<U_PerFrame_InDeferred>& per_frame_in_deferred,
        const UniformBufferArray<U_Material_InDeferred>& material_buffer,
        const VkImageView textureImageView,
        const VkSampler textureSampler,
        const VkDevice logi_device
    ) {
        for (size_t i = 0; i < this->m_handles.size(); i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = per_frame_in_deferred.buffer_at(i);
            bufferInfo.offset = 0;
            bufferInfo.range = per_frame_in_deferred.data_size();

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;

            VkDescriptorBufferInfo buffer_material_info{};
            buffer_material_info.buffer = material_buffer.buffer_at(0);
            buffer_material_info.offset = 0;
            buffer_material_info.range = material_buffer.data_size();


            std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = this->m_handles.at(i);
            descriptorWrites[0].dstBinding = 0;  // specified in shader code
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            descriptorWrites[0].pImageInfo = nullptr; // Optional
            descriptorWrites[0].pTexelBufferView = nullptr; // Optional

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = this->m_handles.at(i);
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = nullptr; // Optional
            descriptorWrites[1].pImageInfo = &imageInfo;
            descriptorWrites[1].pTexelBufferView = nullptr; // Optional

            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = this->m_handles.at(i);
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pBufferInfo = &buffer_material_info;
            descriptorWrites[2].pImageInfo = nullptr;
            descriptorWrites[2].pTexelBufferView = nullptr;


            vkUpdateDescriptorSets(
                logi_device,
                static_cast<uint32_t>(descriptorWrites.size()),
                descriptorWrites.data(),
                0, nullptr
            );
        }
    }

    void DescriptorSet::record_composition(
        const size_t swapchainImagesSize,
        const UniformBufferArray<U_PerFrame_InComposition>& u_per_frame,
        const VkDescriptorSetLayout descriptorSetLayout,
        const std::vector<VkImageView>& attachment_views,
        const std::vector<VkImageView>& dlight_shadow_map_view,
        const VkSampler dlight_shadow_map_sampler,
        const VkDevice logiDevice
    ) {
        for (size_t i = 0; i < swapchainImagesSize; i++) {
            std::vector<VkDescriptorImageInfo> imageInfo(attachment_views.size());
            for (size_t j = 0; j < attachment_views.size(); ++j) {
                auto& x = imageInfo.at(j);
                x.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                x.imageView = attachment_views[j];
                x.sampler = VK_NULL_HANDLE;
            }

            std::vector<VkWriteDescriptorSet> descriptorWrites(imageInfo.size());
            for (size_t j = 0; j < imageInfo.size(); ++j) {
                auto& x = descriptorWrites.at(j);

                x.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                x.dstSet = this->m_handles.at(i);
                x.dstBinding = j;  // specified in shader code
                x.dstArrayElement = 0;
                x.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                x.descriptorCount = 1;
                x.pImageInfo = &imageInfo[j];
            }

            // Unifrom buffer

            VkDescriptorBufferInfo buffer_per_frame_info{};
            buffer_per_frame_info.buffer = u_per_frame.buffer_at(i);
            buffer_per_frame_info.offset = 0;
            buffer_per_frame_info.range = u_per_frame.data_size();
            {
                auto& x = descriptorWrites.emplace_back();
                x.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                x.dstSet = this->m_handles.at(i);
                x.dstBinding = descriptorWrites.size() - 1;
                x.dstArrayElement = 0;
                x.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                x.descriptorCount = 1;
                x.pBufferInfo = &buffer_per_frame_info;
                x.pImageInfo = nullptr;
                x.pTexelBufferView = nullptr;
            }

            std::array<VkDescriptorImageInfo, 3> dlight_shadow_map_info{};
            for (uint32_t i = 0; i < dlight_shadow_map_info.size(); ++i) {
                dlight_shadow_map_info.at(i).imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                dlight_shadow_map_info.at(i).imageView = dlight_shadow_map_view.at(i);
                dlight_shadow_map_info.at(i).sampler = dlight_shadow_map_sampler;
            }
            {
                auto& x = descriptorWrites.emplace_back();
                x.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                x.dstSet = this->m_handles.at(i);
                x.dstBinding = descriptorWrites.size() - 1;
                x.dstArrayElement = 0;
                x.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                x.descriptorCount = dlight_shadow_map_info.size();
                x.pImageInfo = dlight_shadow_map_info.data();
            }

            // Create

            vkUpdateDescriptorSets(
                logiDevice,
                descriptorWrites.size(),
                descriptorWrites.data(),
                0, nullptr
            );
        }
    }

    void DescriptorSet::record_shadow(const VkDevice logi_device) {
        for (size_t i = 0; i < this->m_handles.size(); i++) {
            std::array<VkWriteDescriptorSet, 0> descriptorWrites{};

            vkUpdateDescriptorSets(
                logi_device,
                descriptorWrites.size(),
                descriptorWrites.data(),
                0, nullptr
            );
        }
    }

}


namespace dal {

    void DescriptorPool::initPool(VkDevice logiDevice, size_t swapchainImagesSize) {
        constexpr uint32_t POOL_SIZE_MULTIPLIER = 50;

        std::array<VkDescriptorPoolSize, 3> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(swapchainImagesSize) * POOL_SIZE_MULTIPLIER;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(swapchainImagesSize) * POOL_SIZE_MULTIPLIER;
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        poolSizes[2].descriptorCount = static_cast<uint32_t>(swapchainImagesSize) * POOL_SIZE_MULTIPLIER;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(swapchainImagesSize) * POOL_SIZE_MULTIPLIER;

        if (VK_SUCCESS != vkCreateDescriptorPool(logiDevice, &poolInfo, nullptr, &this->descriptorPool)) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void DescriptorPool::addSets_composition(
        const VkDevice logiDevice,
        const size_t swapchainImagesSize,
        const VkDescriptorSetLayout descriptorSetLayout,
        const UniformBufferArray<U_PerFrame_InComposition>& ubuf_per_frame,
        const std::vector<VkImageView>& attachment_views,
        const std::vector<VkImageView>& dlight_shadow_map_view,
        const VkSampler dlight_shadow_map_sampler
    ) {
        auto& new_one = this->m_descset_composition.emplace_back();
        new_one.init(swapchainImagesSize, descriptorSetLayout, this->descriptorPool, logiDevice);
        new_one.record_composition(swapchainImagesSize, ubuf_per_frame, descriptorSetLayout, attachment_views, dlight_shadow_map_view, dlight_shadow_map_sampler, logiDevice);
    }

    void DescriptorPool::init_descset_shadow(
        const uint32_t swapchain_count,
        const VkDescriptorSetLayout descset_layout,
        const VkDevice logi_device
    ) {
        this->m_descset_shadow.destroy(this->pool(), logi_device);
        this->m_descset_shadow.init(swapchain_count, descset_layout, this->pool(), logi_device);
        this->m_descset_shadow.record_shadow(logi_device);
    }

    void DescriptorPool::destroy(VkDevice logiDevice) {
        if (VK_NULL_HANDLE != this->descriptorPool) {
            vkDestroyDescriptorPool(logiDevice, this->descriptorPool, nullptr);
            this->descriptorPool = VK_NULL_HANDLE;
        }
        this->m_descset_composition.clear();
        this->m_descset_shadow.vector().clear();
    }

    std::vector<std::vector<VkDescriptorSet>> DescriptorPool::descset_composition() const {
        std::vector<std::vector<VkDescriptorSet>> result;

        for (auto& x : this->m_descset_composition) {
            result.emplace_back(x.vector());
        }

        return result;
    }

}
