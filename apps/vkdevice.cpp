#include "vkdevice.h"

#include <set>
#include <tuple>
#include <vector>
#include <optional>
#include <iostream>

#include "konst.h"
#include "util_vulkan.h"

#define DAL_PRINT_DEVICE_INFO true


namespace {

    template <typename T>
    constexpr T clamp(const T v, const T minv, const T maxv) {
        return max(minv, min(maxv, v));
    }

}


namespace {

    const std::array<const char*, 1> DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };


    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    SwapChainSupportDetails querySwapChainSupport(const VkSurfaceKHR surface, const VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if ( 0 != formatCount ) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if ( 0 != presentModeCount ) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
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

        if ( VK_NULL_HANDLE == this->m_handle ) {
            throw std::runtime_error{ "failed to find a sutable graphic device." };
        }
    }

    // static private

    unsigned PhysDevice::rateDeviceSuitability(VkSurfaceKHR surface, VkPhysicalDevice physDevice) {
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

            if ( !findQueueFamilies(physDevice, surface).isComplete() )
                return 0;

            if ( !checkDeviceExtensionSupport(physDevice) )
                return 0;

            const auto swapChainSupport = querySwapChainSupport(surface, physDevice);
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

    bool PhysDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

        for ( const auto& extension : availableExtensions ) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    void PhysDevice::printDeviceInfo(const VkPhysicalDeviceProperties& properties, const VkPhysicalDeviceFeatures& features) {
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

}


// LogiDeviceAndQueue
namespace dal {

    void LogiDeviceAndQueue::init(VkSurfaceKHR surface, VkPhysicalDevice physDevice) {
        const auto indices = findQueueFamilies(physDevice, surface);

        // Create vulkan device
        {
            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            {
                std::set<uint32_t> uniqueQueueFamilies = { indices.m_graphicsFamily.value(), indices.m_presentFamily.value() };

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

        vkGetDeviceQueue(this->m_logiDevice, indices.m_graphicsFamily.value(), 0, &this->m_graphicsQueue);
        vkGetDeviceQueue(this->m_logiDevice, indices.m_presentFamily.value(), 0, &this->m_presentQueue);
    }

    void LogiDeviceAndQueue::destroy(void) {
        // Queue is destoryed implicitly when the corresponding VkDevice is destroyed.

        vkDestroyDevice(this->m_logiDevice, nullptr);
        this->m_logiDevice = VK_NULL_HANDLE;
    }

}


// Swapchain
namespace dal {

    void Swapchain::init(VkSurfaceKHR surface, VkPhysicalDevice physDevice, VkDevice logiDevice) {
        const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(surface, physDevice);

        const VkSurfaceFormatKHR surfaceFormat = this->chooseSwapSurfaceFormat(swapChainSupport.formats);
        const VkPresentModeKHR presentMode = this->chooseSwapPresentMode(swapChainSupport.presentModes);
        const VkExtent2D extent = this->chooseSwapExtent(swapChainSupport.capabilities);

        // Simply sticking to this minimum means that we may sometimes have to wait on the driver to complete
        // internal operations before we can acquire another image to render to. Therefore it is recommended to
        // request at least one more image than the minimum.
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if ( swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount ) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        // Create swap chain
        {
            VkSwapchainCreateInfoKHR createInfo = {};

            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = surface;

            createInfo.minImageCount = imageCount;
            createInfo.imageFormat = surfaceFormat.format;
            createInfo.imageColorSpace = surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            QueueFamilyIndices indices = findQueueFamilies(physDevice, surface);
            uint32_t queueFamilyIndices[] = { indices.m_graphicsFamily.value(), indices.m_presentFamily.value() };

            if ( indices.m_graphicsFamily != indices.m_presentFamily ) {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = 2;
                createInfo.pQueueFamilyIndices = queueFamilyIndices;
            }
            else {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                createInfo.queueFamilyIndexCount = 0; // Optional
                createInfo.pQueueFamilyIndices = nullptr; // Optional
            }

            createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            createInfo.presentMode = presentMode;
            createInfo.clipped = VK_TRUE;
            createInfo.oldSwapchain = VK_NULL_HANDLE;

            if ( vkCreateSwapchainKHR(logiDevice, &createInfo, nullptr, &this->m_swapChain) != VK_SUCCESS ) {
                throw std::runtime_error("failed to create swap chain!");
            }
        }

        this->m_imageFormat = surfaceFormat.format;
        this->m_extent = extent;
    }

    void Swapchain::destroy(VkDevice logiDevice) {
        vkDestroySwapchainKHR(logiDevice, this->m_swapChain, nullptr);
        this->m_swapChain = VK_NULL_HANDLE;
    }

    // Private

    VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for ( const auto& availableFormat : availableFormats ) {
            if ( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for ( const auto& availablePresentMode : availablePresentModes ) {
            // This is the triple buffering.
            if ( VK_PRESENT_MODE_MAILBOX_KHR == availablePresentMode ) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D Swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if ( capabilities.currentExtent.width != UINT32_MAX ) {
            return capabilities.currentExtent;
        }
        else {
            VkExtent2D actualExtent = { dal::WIN_WIDTH, dal::WIN_HEIGHT };

            actualExtent.width = clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

}


// GraphicDevice
namespace dal {

    void GraphicDevice::init(const VkInstance instance, const VkSurfaceKHR surface) {
        this->m_physDevice.init(instance, surface);
        this->m_logiDevice.init(surface, this->m_physDevice.get());
        this->m_swapchain.init(surface, this->m_physDevice.get(), this->m_logiDevice.get());
        this->m_swapchainImages.init(this->m_logiDevice.get(), this->m_swapchain.get(), this->m_swapchain.imageFormat(), this->m_swapchain.extent());
        this->m_renderPass.init(this->m_logiDevice.get(), this->m_swapchain.imageFormat());
        this->m_pipeline.init(this->m_logiDevice.get(), this->m_renderPass.get(), this->m_swapchain.extent());
        this->m_fbuf.init(this->m_logiDevice.get(), this->m_renderPass.get(), this->m_swapchainImages.getViews(), this->m_swapchain.extent());
        this->m_command.init(this->m_physDevice.get(), this->m_logiDevice.get(), surface, this->m_fbuf.getList().size());
    }

    void GraphicDevice::destroy(void) {
        this->m_command.destroy(this->m_logiDevice.get());
        this->m_fbuf.destroy(this->m_logiDevice.get());
        this->m_pipeline.destroy(this->m_logiDevice.get());
        this->m_renderPass.destroy(this->m_logiDevice.get());
        this->m_swapchainImages.destroy(this->m_logiDevice.get());
        this->m_swapchain.destroy(this->m_logiDevice.get());
        this->m_logiDevice.destroy();
    }

}
