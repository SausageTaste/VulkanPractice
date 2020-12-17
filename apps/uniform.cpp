#include "uniform.h"

#include <chrono>
#include <stdexcept>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace {

    uint32_t findMemType(const uint32_t typeFilter, const VkMemoryPropertyFlags props, const VkPhysicalDevice physDevice) {
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(physDevice, &memProps);

        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
            if (typeFilter & (1 << i) && (memProps.memoryTypes[i].propertyFlags & props) == props) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
        VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDevice logiDevice, VkPhysicalDevice physDevice)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(logiDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(logiDevice, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = ::findMemType(
            memRequirements.memoryTypeBits, properties, physDevice
        );

        if (vkAllocateMemory(logiDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(logiDevice, buffer, bufferMemory, 0);
    }

}


namespace dal {

    void DescriptorSetLayout::init(const VkDevice logiDevice) {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (VK_SUCCESS != vkCreateDescriptorSetLayout(logiDevice, &layoutInfo, nullptr, &this->descriptorSetLayout)) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void DescriptorSetLayout::destroy(const VkDevice logiDevice) {
        vkDestroyDescriptorSetLayout(logiDevice, this->descriptorSetLayout, nullptr);
        this->descriptorSetLayout = VK_NULL_HANDLE;
    }

}


namespace dal {

    void DescriptorPool::initPool(VkDevice logiDevice, size_t swapchainImagesSize) {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(swapchainImagesSize);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(swapchainImagesSize);

        if (VK_SUCCESS != vkCreateDescriptorPool(logiDevice, &poolInfo, nullptr, &this->descriptorPool)) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void DescriptorPool::initSets(VkDevice logiDevice, size_t swapchainImagesSize, VkDescriptorSetLayout descriptorSetLayout, const std::vector<VkBuffer>& uniformBuffers) {
        std::vector<VkDescriptorSetLayout> layouts(swapchainImagesSize, descriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = this->descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapchainImagesSize);
        allocInfo.pSetLayouts = layouts.data();

        this->descriptorSets.resize(swapchainImagesSize, VK_NULL_HANDLE);
        if (vkAllocateDescriptorSets(logiDevice, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < swapchainImagesSize; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = this->descriptorSets[i];
            descriptorWrite.dstBinding = 0;  // specified in shader code
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;
            descriptorWrite.pImageInfo = nullptr; // Optional
            descriptorWrite.pTexelBufferView = nullptr; // Optional

            vkUpdateDescriptorSets(logiDevice, 1, &descriptorWrite, 0, nullptr);
        }
    }

    void DescriptorPool::destroy(VkDevice logiDevice) {
        vkDestroyDescriptorPool(logiDevice, this->descriptorPool, nullptr);
        this->descriptorPool = VK_NULL_HANDLE;

        this->descriptorSets.clear();
    }

}


namespace dal {

    void UniformBuffers::init(VkDevice logiDevice, VkPhysicalDevice physDevice, size_t swapchainImagesSize) {
        VkDeviceSize bufferSize = sizeof(dal::UniformBufferObject);

        this->uniformBuffers.resize(swapchainImagesSize);
        this->uniformBuffersMemory.resize(swapchainImagesSize);

        for (size_t i = 0; i < swapchainImagesSize; ++i) {
            ::createBuffer(
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
