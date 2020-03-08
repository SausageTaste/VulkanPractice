#include "vkdevice.h"

#include <vector>
#include <optional>
#include <iostream>

#include "vkextension.h"

#define DAL_PRINT_DEVICE_INFO false


namespace {

    struct QueueFamilyIndices {
        std::optional<uint32_t> m_graphicsFamily;

        bool isComplete(void) const {
            return this->m_graphicsFamily.has_value();
        }
    };

    QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for ( const auto& queueFamily : queueFamilies ) {
            if ( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
                indices.m_graphicsFamily = i;

            if ( indices.isComplete() )
                break;

            i++;
        }

        return indices;
    }


    void printDeviceInfo(const VkPhysicalDeviceProperties& properties, const VkPhysicalDeviceFeatures& features) {
        std::cout << "physical device \"" << properties.deviceName << "\"\n";

        std::cout << "         type                     : ";
        switch ( properties.deviceType ) {

        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            std::cout << "integrated gpu"; break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            std::cout << "discrete gpu"; break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            std::cout << "virtual gpu"; break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            std::cout << "cpu"; break;
        default:
            std::cout << "unknown"; break;

        }
        std::cout << '\n';

        std::cout << "         vendor                   : ";
        switch ( properties.vendorID ) {

        case 0x1002:
            std::cout << "AMD"; break;

        case 0x1010:
            std::cout << "ImgTec"; break;

        case 0x10DE:
            std::cout << "NVIDIA"; break;

        case 0x13B5:
            std::cout << "ARM"; break;

        case 0x5143:
            std::cout << "Qualcomm"; break;

        case 0x8086:
            std::cout << "INTEL"; break;

        }
        std::cout << '\n';

        std::cout << "         max memory alloc count   : " << properties.limits.maxMemoryAllocationCount << '\n';
        std::cout << "         max sampler alloc count  : " << properties.limits.maxSamplerAllocationCount << '\n';
        std::cout << "         max image 2d dimension   : " << properties.limits.maxImageDimension2D << '\n';
        std::cout << "         max image cube dimension : " << properties.limits.maxImageDimensionCube << '\n';
    }

    unsigned rateDeviceSuitability(VkPhysicalDevice device) {
        unsigned score = 0;

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);

#if DAL_PRINT_DEVICE_INFO
        printDeviceInfo(properties, features);
#endif

        {
            // Discrete GPUs have a significant performance advantage
            if ( VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == properties.deviceType )
                score += 5000;

            // Maximum possible size of textures affects graphics quality
            score += properties.limits.maxImageDimension2D;

            // Application can't function without geometry shaders
            if ( !features.geometryShader )
                return 0;

            if ( !findQueueFamilies(device).isComplete() )
                return 0;
        }

#if DAL_PRINT_DEVICE_INFO
        std::cout << "         score                    : " << score << '\n';
#endif

        return score;
    }

    VkPhysicalDevice pickDevice(const VkInstance instance) {
        auto deviceCount = [instance](void) {
            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

            if ( deviceCount == 0 ) {
                throw std::runtime_error{ "failed to find GPUs with Vulkan support!" };
            }
            else {
                std::cout << "There are " << deviceCount << " devices found.\n";
            }

            return deviceCount;
        }();

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        VkPhysicalDevice seleted = VK_NULL_HANDLE;
        unsigned bestScore = -1;

        for ( auto device : devices ) {
            const auto score = rateDeviceSuitability(device);
            if ( score < bestScore ) {
                seleted = device;
                bestScore = score;
            }
        }

        return seleted;
    }


    std::pair<VkDevice, VkQueue> createLogicalDevice(const VkPhysicalDevice physicalDevice) {
        std::pair<VkDevice, VkQueue> result{ VK_NULL_HANDLE, VK_NULL_HANDLE };

        const auto indices = findQueueFamilies(physicalDevice);

        VkDeviceQueueCreateInfo queueCreateInfo = {};
        {
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = indices.m_graphicsFamily.value();
            queueCreateInfo.queueCount = 1;

            const float queuePriority = 1.f;
            queueCreateInfo.pQueuePriorities = &queuePriority;
        }

        VkPhysicalDeviceFeatures deviceFeatures = {};

        VkDeviceCreateInfo createInfo = {};
        {
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

            createInfo.pQueueCreateInfos = &queueCreateInfo;
            createInfo.queueCreateInfoCount = 1;
            createInfo.pEnabledFeatures = &deviceFeatures;

            createInfo.enabledExtensionCount = 0;

#ifndef NDEBUG
            createInfo.enabledLayerCount = static_cast<uint32_t>(dal::VAL_LAYERS_TO_USE.size());
            createInfo.ppEnabledLayerNames = dal::VAL_LAYERS_TO_USE.data();
#else
            createInfo.enabledLayerCount = 0;
#endif
        }

        if ( VK_SUCCESS != vkCreateDevice(physicalDevice, &createInfo, nullptr, &result.first) ) {
            throw std::runtime_error{ "failed to create logical device!" };
        }

        vkGetDeviceQueue(result.first, indices.m_graphicsFamily.value(), 0, &result.second);

        return result;
    }

}


namespace dal {

    GraphicDevice::~GraphicDevice(void) {
        this->destroy();
    }

    void GraphicDevice::init(const VkInstance instance) {
        this->m_physicalDevice = pickDevice(instance);
        if ( VK_NULL_HANDLE == this->m_physicalDevice ) {
            throw std::runtime_error{ "failed to find a sutable graphic device." };
        }

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(this->m_physicalDevice, &properties);
        std::cout << "Selected graphic device: " << properties.deviceName << '\n';

        std::tie(this->m_logicalDevice, this->m_graphicsQueue) = createLogicalDevice(this->m_physicalDevice);
    }

    void GraphicDevice::destroy(void) {
        vkDestroyDevice(this->m_logicalDevice, nullptr);
        this->m_logicalDevice = VK_NULL_HANDLE;

        // Physical device is destoryed implicitly when the corresponding VkInstance is destroyed.
        // Queue is destoryed implicitly when the corresponding VkDevice is destroyed.
    }

}
