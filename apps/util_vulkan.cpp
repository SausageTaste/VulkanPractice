#include "util_vulkan.h"

#include <vector>


namespace dal {

    QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice device, const VkSurfaceKHR surface) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for ( const auto& queueFamily : queueFamilies ) {
            if ( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
                indices.m_graphicsFamily = i;

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if ( presentSupport ) {
                indices.m_presentFamily = i;
            }

            if ( indices.isComplete() )
                break;

            i++;
        }

        return indices;
    }

}
