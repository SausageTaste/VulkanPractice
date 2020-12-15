#include "logidevice.h"

#include <set>
#include <vector>
#include <stdexcept>

#include "konst.h"
#include "util_vulkan.h"


// LogiDeviceAndQueue
namespace dal {

    void LogiDeviceAndQueue::init(VkSurfaceKHR surface, VkPhysicalDevice physDevice) {
        const auto indices = findQueueFamilies(physDevice, surface);

        // Create vulkan device
        {
            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            {
                std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily(), indices.presentFamily() };

                float queuePriority = 1.f;
                for ( const uint32_t queueFamily : uniqueQueueFamilies ) {
                    VkDeviceQueueCreateInfo queueCreateInfo = {};
                    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    queueCreateInfo.queueFamilyIndex = queueFamily;
                    queueCreateInfo.queueCount = 1;
                    queueCreateInfo.pQueuePriorities = &queuePriority;
                    queueCreateInfos.push_back(queueCreateInfo);
                }
            }

            VkPhysicalDeviceFeatures deviceFeatures = {};

            VkDeviceCreateInfo createInfo = {};
            {
                createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

                createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
                createInfo.pQueueCreateInfos = queueCreateInfos.data();

                createInfo.pEnabledFeatures = &deviceFeatures;

                createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
                createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

#ifndef NDEBUG
                createInfo.enabledLayerCount = static_cast<uint32_t>(dal::VAL_LAYERS_TO_USE.size());
                createInfo.ppEnabledLayerNames = dal::VAL_LAYERS_TO_USE.data();
#else
                createInfo.enabledLayerCount = 0;
#endif
            }

            if ( VK_SUCCESS != vkCreateDevice(physDevice, &createInfo, nullptr, &this->m_logiDevice) ) {
                throw std::runtime_error{ "failed to create logical device!" };
            }
        }

        vkGetDeviceQueue(this->m_logiDevice, indices.graphicsFamily(), 0, &this->m_graphicsQueue);
        vkGetDeviceQueue(this->m_logiDevice, indices.presentFamily(), 0, &this->m_presentQueue);
    }

    void LogiDeviceAndQueue::destroy(void) {
        // Queue is destoryed implicitly when the corresponding VkDevice is destroyed.

        vkDestroyDevice(this->m_logiDevice, nullptr);
        this->m_logiDevice = VK_NULL_HANDLE;
    }

}
