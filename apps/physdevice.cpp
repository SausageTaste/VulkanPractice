#include "physdevice.h"

#include <set>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

#include "util_vulkan.h"
#include "konst.h"

#define DAL_PRINT_DEVICE_INFO true


namespace {

    bool does_support_astc_texture(VkPhysicalDevice physDevice) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physDevice, VK_FORMAT_ASTC_4x4_UNORM_BLOCK, &properties);
        return (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(dal::DEVICE_EXTENSIONS.begin(), dal::DEVICE_EXTENSIONS.end());

        for ( const auto& extension : availableExtensions ) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
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

    unsigned rateDeviceSuitability(VkSurfaceKHR surface, VkPhysicalDevice physDevice) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physDevice, &properties);
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physDevice, &features);

#if DAL_PRINT_DEVICE_INFO
        printDeviceInfo(properties, features);
#endif

        // Invalid device condition
        {
            // Application can't function without geometry shaders
            if ( !features.geometryShader )
                return 0;

            if ( !features.samplerAnisotropy )
                return 0;

            if ( !::does_support_astc_texture(physDevice) )
                return 0;

            if ( !dal::findQueueFamilies(physDevice, surface).isComplete() )
                return 0;

            if ( !checkDeviceExtensionSupport(physDevice) )
                return 0;

            const auto swapChainSupport = dal::querySwapChainSupport(surface, physDevice);
            const auto swapChainAdequate = !(swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty());
            if ( !swapChainAdequate )
                return 0;
        }

        unsigned score = 0;
        {
            // Discrete GPUs have a significant performance advantage
            if ( VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == properties.deviceType )
                score += 5000;

            // Maximum possible size of textures affects graphics quality
            score += properties.limits.maxImageDimension2D;
        }

#if DAL_PRINT_DEVICE_INFO
        std::cout << "         score                    : " << score << '\n';
#endif

        return score;
    }

}


// PhysDevice
namespace dal {

    void PhysDevice::init(VkInstance instance, VkSurfaceKHR surface) {
        auto deviceCount = [instance](void) {
            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

            if ( deviceCount == 0 ) {
                throw std::runtime_error{ "failed to find GPUs with Vulkan support!" };
            }

            return deviceCount;
        }();

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        unsigned bestScore = -1;

        for ( auto device : devices ) {
            const auto score = rateDeviceSuitability(surface, device);
            if ( score < bestScore ) {
                this->m_handle = device;
                bestScore = score;
            }
        }

#if DAL_PRINT_DEVICE_INFO
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(this->m_handle, &properties);
        std::cout << "Selected GPU: " << properties.deviceName << '\n';
#endif

        if ( VK_NULL_HANDLE == this->m_handle ) {
            throw std::runtime_error{ "failed to find a sutable graphic device." };
        }
    }

}
