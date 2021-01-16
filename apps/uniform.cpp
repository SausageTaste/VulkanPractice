#include "uniform.h"

#include <array>
#include <chrono>
#include <stdexcept>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#include "util_vulkan.h"


namespace dal {

    std::pair<VkBuffer, VkDeviceMemory> _create_uniform_buffer_memory(const uint32_t data_size, const VkDevice logi_device, const VkPhysicalDevice phys_device) {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;

        dal::createBuffer(
            data_size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            buffer, memory,
            logi_device, phys_device
        );

        return std::make_pair(buffer, memory);
    }

}


namespace {

    VkDescriptorSetLayout create_layout_deferred(const VkDevice logiDevice) {
        std::array<VkDescriptorSetLayoutBinding, 4> bindings{};

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

        bindings.at(3).binding = 3;
        bindings.at(3).descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings.at(3).descriptorCount = 1;
        bindings.at(3).stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

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

    void DescSet::record_deferred(
        const UniformBuffer<U_PerFrame_InDeferred>& ubuf_per_frame_in_deferred,
        const UniformBuffer<U_Material_InDeferred>& ubuf_material,
        const UniformBuffer<U_PerInst_PerFrame_InDeferred>& ubuf_per_inst_per_frame,
        const VkImageView textureImageView,
        const VkSampler textureSampler,
        const VkDevice logi_device
    ) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = ubuf_per_frame_in_deferred.buffer();
        bufferInfo.offset = 0;
        bufferInfo.range = ubuf_per_frame_in_deferred.data_size();

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        VkDescriptorBufferInfo buffer_material_info{};
        buffer_material_info.buffer = ubuf_material.buffer();
        buffer_material_info.offset = 0;
        buffer_material_info.range = ubuf_material.data_size();

        VkDescriptorBufferInfo ubuf_info_per_inst_per_frame{};
        ubuf_info_per_inst_per_frame.buffer = ubuf_per_inst_per_frame.buffer();
        ubuf_info_per_inst_per_frame.offset = 0;
        ubuf_info_per_inst_per_frame.range = ubuf_per_inst_per_frame.data_size();


        std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = this->m_handle;
        descriptorWrites[0].dstBinding = 0;  // specified in shader code
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        descriptorWrites[0].pImageInfo = nullptr; // Optional
        descriptorWrites[0].pTexelBufferView = nullptr; // Optional

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = this->m_handle;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = nullptr; // Optional
        descriptorWrites[1].pImageInfo = &imageInfo;
        descriptorWrites[1].pTexelBufferView = nullptr; // Optional

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = this->m_handle;
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &buffer_material_info;
        descriptorWrites[2].pImageInfo = nullptr;
        descriptorWrites[2].pTexelBufferView = nullptr;

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = this->m_handle;
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pBufferInfo = &ubuf_info_per_inst_per_frame;


        vkUpdateDescriptorSets(
            logi_device,
            static_cast<uint32_t>(descriptorWrites.size()),
            descriptorWrites.data(),
            0, nullptr
        );
    }

    void DescSet::record_composition(
        const size_t swapchainImagesSize,
        const UniformBuffer<U_PerFrame_InComposition> ubuf_per_frame,
        const VkDescriptorSetLayout descriptorSetLayout,
        const std::vector<VkImageView>& attachment_views,
        const std::vector<VkImageView>& dlight_shadow_map_view,
        const VkSampler dlight_shadow_map_sampler,
        const VkDevice logiDevice
    ) {
        std::vector<VkDescriptorImageInfo> imageInfo(attachment_views.size());
        for (size_t i = 0; i < attachment_views.size(); ++i) {
            auto& x = imageInfo.at(i);
            x.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            x.imageView = attachment_views[i];
            x.sampler = VK_NULL_HANDLE;
        }

        std::vector<VkWriteDescriptorSet> descriptorWrites(imageInfo.size());
        for (size_t i = 0; i < imageInfo.size(); ++i) {
            auto& x = descriptorWrites.at(i);

            x.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            x.dstSet = this->m_handle;
            x.dstBinding = i;  // specified in shader code
            x.dstArrayElement = 0;
            x.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            x.descriptorCount = 1;
            x.pImageInfo = &imageInfo[i];
        }

        // Unifrom buffer

        VkDescriptorBufferInfo buffer_per_frame_info{};
        buffer_per_frame_info.buffer = ubuf_per_frame.buffer();
        buffer_per_frame_info.offset = 0;
        buffer_per_frame_info.range = ubuf_per_frame.data_size();
        {
            auto& x = descriptorWrites.emplace_back();
            x.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            x.dstSet = this->m_handle;
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
            x.dstSet = this->m_handle;
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

    void DescSet::record_shadow(const VkDevice logi_device) {
        std::array<VkWriteDescriptorSet, 0> descriptorWrites{};

        vkUpdateDescriptorSets(
            logi_device,
            descriptorWrites.size(),
            descriptorWrites.data(),
            0, nullptr
        );
    }

}


namespace dal {

    void DescPool::init(
        const uint32_t uniform_buf_count,
        const uint32_t image_sampler_count,
        const uint32_t input_attachment_count,
        const uint32_t desc_set_count,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        std::array<VkDescriptorPoolSize, 3> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = uniform_buf_count;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = image_sampler_count;
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        poolSizes[2].descriptorCount = input_attachment_count;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = desc_set_count;

        if (VK_SUCCESS != vkCreateDescriptorPool(logi_device, &poolInfo, nullptr, &this->m_pool)) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void DescPool::destroy(const VkDevice logi_device) {
        if (VK_NULL_HANDLE != this->m_pool) {
            vkDestroyDescriptorPool(logi_device, this->m_pool, nullptr);
            this->m_pool = VK_NULL_HANDLE;
        }
    }

    void DescPool::reset(const VkDevice logi_device) {
        if (VK_SUCCESS != vkResetDescriptorPool(logi_device, this->m_pool, 0)) {
            throw std::runtime_error{ "failed to reset descriptor pool" };
        }
    }

    DescSet DescPool::allocate(const VkDescriptorSetLayout layout, const VkDevice logi_device) {
        return this->allocate(1, layout, logi_device).at(0);
    }

    std::vector<DescSet> DescPool::allocate(const uint32_t count, const VkDescriptorSetLayout layout, const VkDevice logi_device) {
        std::vector<DescSet> result;

        const std::vector<VkDescriptorSetLayout> layouts(count, layout);

        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = this->get();
        alloc_info.descriptorSetCount = count;
        alloc_info.pSetLayouts = layouts.data();

        std::vector<VkDescriptorSet> desc_sets(count);
        if (VK_SUCCESS != vkAllocateDescriptorSets(logi_device, &alloc_info, desc_sets.data())) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        result.reserve(count);
        for (const auto x : desc_sets) {
            result.emplace_back().set(x);
        }

        return result;
    }

}


namespace dal {

    void DescriptorSetManager::init(const uint32_t swapchain_count, const VkDevice logi_device) {
        constexpr uint32_t POOL_SIZE_MULTIPLIER = 50;

        this->m_pool.init(
            swapchain_count * POOL_SIZE_MULTIPLIER,
            swapchain_count * POOL_SIZE_MULTIPLIER,
            swapchain_count * POOL_SIZE_MULTIPLIER,
            swapchain_count * POOL_SIZE_MULTIPLIER,
            logi_device
        );
    }

    void DescriptorSetManager::addSets_composition(
        const VkDevice logiDevice,
        const size_t swapchainImagesSize,
        const VkDescriptorSetLayout descriptorSetLayout,
        const UniformBufferArray<U_PerFrame_InComposition>& ubuf_per_frame,
        const std::vector<VkImageView>& attachment_views,
        const std::vector<VkImageView>& dlight_shadow_map_view,
        const VkSampler dlight_shadow_map_sampler
    ) {
        auto desc_sets = this->m_pool.allocate(swapchainImagesSize, descriptorSetLayout, logiDevice);

        for (uint32_t i = 0; i < desc_sets.size(); ++i) {
            desc_sets.at(i).record_composition(
                swapchainImagesSize,
                ubuf_per_frame.buffer_at(i),
                descriptorSetLayout,
                attachment_views,
                dlight_shadow_map_view,
                dlight_shadow_map_sampler,
                logiDevice
            );
        }

        this->m_descset_composition.emplace_back(desc_sets);
    }

    void DescriptorSetManager::init_descset_shadow(
        const uint32_t swapchain_count,
        const VkDescriptorSetLayout descset_layout,
        const VkDevice logi_device
    ) {
        this->m_descset_shadow = this->m_pool.allocate(swapchain_count, descset_layout, logi_device);

        for (uint32_t i = 0; i < this->m_descset_shadow.size(); ++i) {
            this->m_descset_shadow.at(i).record_shadow(logi_device);
        }
    }

    void DescriptorSetManager::destroy(VkDevice logiDevice) {
        this->m_pool.destroy(logiDevice);
        this->m_descset_composition.clear();
        this->m_descset_shadow.clear();
    }

    std::vector<std::vector<VkDescriptorSet>> DescriptorSetManager::descset_composition() const {
        std::vector<std::vector<VkDescriptorSet>> result;

        for (auto& x : this->m_descset_composition) {
            auto& one = result.emplace_back();

            for (auto& y : x) {
                one.emplace_back(y.get());
            }
        }

        return result;
    }

    std::vector<VkDescriptorSet> DescriptorSetManager::descset_shadow() const {
        std::vector<VkDescriptorSet> result;

        for (auto& x : this->m_descset_shadow) {
            result.emplace_back(x.get());
        }

        return result;
    }

}
