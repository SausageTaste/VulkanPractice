#pragma once

#include <optional>
#include <cstdint>

#include <vulkan/vulkan.h>


namespace dal {

    struct QueueFamilyIndices {
        std::optional<uint32_t> m_graphicsFamily;
        std::optional<uint32_t> m_presentFamily;

        bool isComplete(void) const {
            if ( !this->m_graphicsFamily.has_value() ) return false;
            if ( !this->m_presentFamily.has_value() ) return false;

            return true;
        }
    };

    QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice device, const VkSurfaceKHR surface);

}
