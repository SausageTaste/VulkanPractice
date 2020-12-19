#include "uniform.h"

#include <array>
#include <chrono>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

#include "util_vulkan.h"


namespace dal {

    void DescriptorSetLayout::init(const VkDevice logiDevice) {
        std::array<VkDescriptorSetLayoutBinding, 2> bindings{};

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

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logiDevice, &layoutInfo, nullptr, &this->descriptorSetLayout)) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void DescriptorSetLayout::destroy(const VkDevice logiDevice) {
        if (VK_NULL_HANDLE != this->descriptorSetLayout) {
            vkDestroyDescriptorSetLayout(logiDevice, this->descriptorSetLayout, nullptr);
            this->descriptorSetLayout = VK_NULL_HANDLE;
        }
    }

}


namespace dal {

    void DescriptorPool::initPool(VkDevice logiDevice, size_t swapchainImagesSize) {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(swapchainImagesSize);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(swapchainImagesSize);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(swapchainImagesSize);

        if (VK_SUCCESS != vkCreateDescriptorPool(logiDevice, &poolInfo, nullptr, &this->descriptorPool)) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void DescriptorPool::addSets(
        VkDevice logiDevice, size_t swapchainImagesSize, VkDescriptorSetLayout descriptorSetLayout,
        const std::vector<VkBuffer>& uniformBuffers, VkImageView textureImageView, VkSampler textureSampler
    ) {
        std::vector<VkDescriptorSetLayout> layouts(swapchainImagesSize, descriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = this->descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapchainImagesSize);
        allocInfo.pSetLayouts = layouts.data();

        this->descriptorSetsList.emplace_back();
        auto& new_sets = this->descriptorSetsList.back();

        new_sets.resize(swapchainImagesSize, VK_NULL_HANDLE);
        if (vkAllocateDescriptorSets(logiDevice, &allocInfo, new_sets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < swapchainImagesSize; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = new_sets[i];
            descriptorWrites[0].dstBinding = 0;  // specified in shader code
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            descriptorWrites[0].pImageInfo = nullptr; // Optional
            descriptorWrites[0].pTexelBufferView = nullptr; // Optional

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = new_sets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = nullptr; // Optional
            descriptorWrites[1].pImageInfo = &imageInfo;
            descriptorWrites[1].pTexelBufferView = nullptr; // Optional

            vkUpdateDescriptorSets(
                logiDevice,
                static_cast<uint32_t>(descriptorWrites.size()),
                descriptorWrites.data(),
                0, nullptr
            );
        }
    }

    void DescriptorPool::destroy(VkDevice logiDevice) {
        if (VK_NULL_HANDLE != this->descriptorPool) {
            vkDestroyDescriptorPool(logiDevice, this->descriptorPool, nullptr);
            this->descriptorPool = VK_NULL_HANDLE;
        }

        this->descriptorSetsList.clear();
    }

}


namespace dal {

    void UniformBuffers::init(VkDevice logiDevice, VkPhysicalDevice physDevice, size_t swapchainImagesSize) {
        VkDeviceSize bufferSize = sizeof(dal::UniformBufferObject);

        this->uniformBuffers.resize(swapchainImagesSize);
        this->uniformBuffersMemory.resize(swapchainImagesSize);

        for (size_t i = 0; i < swapchainImagesSize; ++i) {
            dal::createBuffer(
                bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                this->uniformBuffers[i],
                this->uniformBuffersMemory[i],
                logiDevice,
                physDevice
            );
        }
    }

    void UniformBuffers::destroy(const VkDevice logiDevice) {
        for (size_t i = 0; i < this->uniformBuffers.size(); ++i) {
            vkDestroyBuffer(logiDevice, this->uniformBuffers[i], nullptr);
            this->uniformBuffers[i] = VK_NULL_HANDLE;
        }
        for (size_t i = 0; i < this->uniformBuffersMemory.size(); ++i) {
            vkFreeMemory(logiDevice, this->uniformBuffersMemory[i], nullptr);
            this->uniformBuffersMemory[i] = VK_NULL_HANDLE;
        }

        this->uniformBuffers.clear();
        this->uniformBuffersMemory.clear();
    }

    void UniformBuffers::update(const uint32_t imageIndex, const VkExtent2D swapchainExtent, const VkDevice logiDevice) {
        static const auto startTime = std::chrono::high_resolution_clock::now();

        const auto currentTime = std::chrono::high_resolution_clock::now();
        const float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        const float ratio = static_cast<float>(swapchainExtent.width) / static_cast<float>(swapchainExtent.height);

        dal::UniformBufferObject ubo;
        ubo.model = glm::rotate(glm::mat4(1), time * glm::radians<float>(90), glm::vec3(0, 1, 0));
        ubo.view = glm::lookAt(glm::vec3(0, 2, 4), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        ubo.proj = glm::perspective<float>(glm::radians<float>(45), ratio, 0, 10);
        ubo.proj[1][1] *= -1;

        void* data;
        vkMapMemory(logiDevice, uniformBuffersMemory[imageIndex], 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(logiDevice, uniformBuffersMemory[imageIndex]);
    }

}
