#include "vkdevice.h"

#include <set>
#include <tuple>
#include <vector>
#include <optional>
#include <iostream>

#include "konst.h"

#define DAL_PRINT_DEVICE_INFO false


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


    struct QueueFamilyIndices {
        std::optional<uint32_t> m_graphicsFamily;
        std::optional<uint32_t> m_presentFamily;

        bool isComplete(void) const {
            if ( !this->m_graphicsFamily.has_value() ) return false;
            if ( !this->m_presentFamily.has_value() ) return false;

            return true;
        }
    };

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

    bool checkDeviceExtensionSupport(const VkPhysicalDevice device) {
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

    unsigned rateDeviceSuitability(const VkSurfaceKHR surface, const VkPhysicalDevice device) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);

#if DAL_PRINT_DEVICE_INFO
        printDeviceInfo(properties, features);
#endif

        // Invalid device condition
        {
            // Application can't function without geometry shaders
            if ( !features.geometryShader )
                return 0;

            if ( !findQueueFamilies(device, surface).isComplete() )
                return 0;

            if ( !checkDeviceExtensionSupport(device) )
                return 0;

            const auto swapChainSupport = querySwapChainSupport(surface, device);
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

    VkPhysicalDevice pickDevice(const VkInstance instance, const VkSurfaceKHR surface) {
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

        VkPhysicalDevice seleted = VK_NULL_HANDLE;
        unsigned bestScore = -1;

        for ( auto device : devices ) {
            const auto score = rateDeviceSuitability(surface, device);
            if ( score < bestScore ) {
                seleted = device;
                bestScore = score;
            }
        }

        return seleted;
    }

    std::tuple<VkDevice, VkQueue, VkQueue> createLogicalDevice(const VkSurfaceKHR surface, const VkPhysicalDevice physicalDevice) {
        const auto indices = findQueueFamilies(physicalDevice, surface);

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

        VkDevice logicalDevice = VK_NULL_HANDLE;
        if ( VK_SUCCESS != vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) ) {
            throw std::runtime_error{ "failed to create logical device!" };
        }

        VkQueue graphicQ = VK_NULL_HANDLE, presentQ = VK_NULL_HANDLE;
        vkGetDeviceQueue(logicalDevice, indices.m_graphicsFamily.value(), 0, &graphicQ);
        vkGetDeviceQueue(logicalDevice, indices.m_presentFamily.value(), 0, &presentQ);

        return std::make_tuple(logicalDevice, graphicQ, presentQ);
    }


    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for ( const auto& availableFormat : availableFormats ) {
            if ( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for ( const auto& availablePresentMode : availablePresentModes ) {
            // This is the triple buffering.
            if ( VK_PRESENT_MODE_MAILBOX_KHR == availablePresentMode ) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
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

    std::tuple<VkSwapchainKHR, VkFormat, VkExtent2D> createSwapChain(const VkSurfaceKHR surface, const VkPhysicalDevice physicalDevice, const VkDevice logicalDevice)
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(surface, physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        // Simply sticking to this minimum means that we may sometimes have to wait on the driver to complete
        // internal operations before we can acquire another image to render to. Therefore it is recommended to
        // request at least one more image than the minimum.
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if ( swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount ) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo = {};
        {
            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = surface;

            createInfo.minImageCount = imageCount;
            createInfo.imageFormat = surfaceFormat.format;
            createInfo.imageColorSpace = surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
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
        }

        VkSwapchainKHR swapChain = VK_NULL_HANDLE;
        if ( vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS ) {
            throw std::runtime_error("failed to create swap chain!");
        }

        return std::make_tuple(swapChain, surfaceFormat.format, extent);
    }

}


namespace dal {

    GraphicDevice::~GraphicDevice(void) {
        this->destroy();
    }

    void GraphicDevice::init(const VkInstance instance, const VkSurfaceKHR surface) {
        this->m_physicalDevice = pickDevice(instance, surface);
        if ( VK_NULL_HANDLE == this->m_physicalDevice ) {
            throw std::runtime_error{ "failed to find a sutable graphic device." };
        }

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(this->m_physicalDevice, &properties);
        std::cout << "Selected graphic device: " << properties.deviceName << '\n';

        std::tie(this->m_logicalDevice, this->m_graphicsQueue, this->m_presentQueue) = createLogicalDevice(surface, this->m_physicalDevice);
        std::tie(this->m_swapChain, this->m_swapChainImageFormat, this->m_swapChainExtent) = createSwapChain(surface, this->m_physicalDevice, this->m_logicalDevice);

        // Get swap chain images.
        {
            uint32_t imageCount;
            vkGetSwapchainImagesKHR(this->m_logicalDevice, this->m_swapChain, &imageCount, nullptr);
            this->m_swapChainImages.resize(imageCount);
            vkGetSwapchainImagesKHR(this->m_logicalDevice, this->m_swapChain, &imageCount, this->m_swapChainImages.data());
        }

        // Create image views
        {
            this->m_swapChainImageViews.resize(this->m_swapChainImages.size());
            for ( size_t i = 0; i < this->m_swapChainImages.size(); i++ ) {
                VkImageViewCreateInfo createInfo = {};
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                createInfo.image = this->m_swapChainImages[i];
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                createInfo.format = this->m_swapChainImageFormat;

                createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

                createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                createInfo.subresourceRange.baseMipLevel = 0;
                createInfo.subresourceRange.levelCount = 1;
                createInfo.subresourceRange.baseArrayLayer = 0;
                createInfo.subresourceRange.layerCount = 1;

                if ( vkCreateImageView(this->m_logicalDevice, &createInfo, nullptr, &this->m_swapChainImageViews[i]) != VK_SUCCESS ) {
                    throw std::runtime_error("failed to create image views!");
                }
            }
        }
    }

    void GraphicDevice::destroy(void) {
        for ( auto imageView : this->m_swapChainImageViews ) {
            vkDestroyImageView(this->m_logicalDevice, imageView, nullptr);
        }
        this->m_swapChainImageViews.clear();

        vkDestroySwapchainKHR(this->m_logicalDevice, this->m_swapChain, nullptr);
        this->m_swapChain = VK_NULL_HANDLE;

        vkDestroyDevice(this->m_logicalDevice, nullptr);
        this->m_logicalDevice = VK_NULL_HANDLE;

        // Physical device is destoryed implicitly when the corresponding VkInstance is destroyed.
        // Queue is destoryed implicitly when the corresponding VkDevice is destroyed.
    }

}
