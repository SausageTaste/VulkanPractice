#pragma once

#include <vector>
#include <cstdint>

#include <vulkan/vulkan.h>


namespace dal {

    class QueueFamilyIndices {

    private:
        static constexpr uint32_t NULL_VAL = -1;

    private:
        uint32_t m_graphicsFamily = NULL_VAL;
        uint32_t m_presentFamily = NULL_VAL;

    public:
        bool isComplete(void) const {
            if ( this->NULL_VAL == this->m_graphicsFamily ) return false;
            if ( this->NULL_VAL == this->m_presentFamily ) return false;

            return true;
        }

        uint32_t graphicsFamily(void) const;
        uint32_t presentFamily(void) const;

        void setGraphicsFamily(const uint32_t v) {
            this->m_graphicsFamily = v;
        }
        void setPresentFamily(const uint32_t v) {
            this->m_presentFamily = v;
        }

    };

    QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice device, const VkSurfaceKHR surface);


    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    SwapChainSupportDetails querySwapChainSupport(const VkSurfaceKHR surface, const VkPhysicalDevice device);


    uint32_t findMemType(const uint32_t typeFilter, const VkMemoryPropertyFlags props, const VkPhysicalDevice physDevice);

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
        VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDevice logiDevice, VkPhysicalDevice physDevice);

    VkDeviceSize createImage(
        uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
        VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
        VkDeviceMemory& imageMemory, VkDevice logiDevice, VkPhysicalDevice physDevice
    );

    VkImageView createImageView(const VkImage image, const VkFormat format, VkImageAspectFlags aspectFlags, const VkDevice logiDevice);

}
