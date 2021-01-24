#include <set>
#include <string>
#include <vector>
#include <array>
#include <iostream>

#include <jni.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include "vulkan_wrapper.h"


#define ALOGI(...) (__android_log_print(ANDROID_LOG_INFO, "vulkan_practice", __VA_ARGS__))


namespace {

    const std::array<const char*, 1> DEVICE_EXTENSIONS = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };


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

        uint32_t graphicsFamily(void) const {
            if ( this->NULL_VAL == this->m_graphicsFamily ) {
                throw std::runtime_error{ "graphics family hasn't been set!" };
            }
            return this->m_graphicsFamily;
        }

        uint32_t presentFamily(void) const {
            if ( this->NULL_VAL == this->m_presentFamily ) {
                throw std::runtime_error{ "present family hasn't been set!" };
            }
            return this->m_presentFamily;
        }

        void setGraphicsFamily(const uint32_t v) {
            this->m_graphicsFamily = v;
        }
        void setPresentFamily(const uint32_t v) {
            this->m_presentFamily = v;
        }

    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
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
                indices.setGraphicsFamily(i);

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if ( presentSupport ) {
                indices.setPresentFamily(i);
            }

            if ( indices.isComplete() )
                break;

            i++;
        }

        return indices;
    }

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


    class PhysDeviceProps {

    private:
        VkPhysicalDevice m_phys_device = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;

        VkPhysicalDeviceProperties m_properties;
        VkPhysicalDeviceFeatures m_features;
        std::vector<VkExtensionProperties> m_available_extensions;

        uint32_t m_score = 0;

    public:
        PhysDeviceProps() = default;

        PhysDeviceProps(const VkPhysicalDevice physDevice, const VkSurfaceKHR surface)
            : m_phys_device(physDevice)
            , m_surface(surface)
        {
            vkGetPhysicalDeviceProperties(this->m_phys_device, &this->m_properties);
            vkGetPhysicalDeviceFeatures(this->m_phys_device, &this->m_features);

            {
                uint32_t extension_count;
                vkEnumerateDeviceExtensionProperties(this->m_phys_device, nullptr, &extension_count, nullptr);
                this->m_available_extensions.resize(extension_count);
                vkEnumerateDeviceExtensionProperties(this->m_phys_device, nullptr, &extension_count, this->m_available_extensions.data());
            }

            this->m_score = this->calc_score();
        }

        auto& props() const {
            return this->m_properties;
        }
        auto& features() const {
            return this->m_features;
        }

        uint32_t score() const {
            return this->m_score;
        }

        void print_info() const {
            ALOGI("physical device \"%a\"", this->m_properties.deviceName);

            switch ( this->m_properties.deviceType ) {
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    ALOGI("\ttype                     : integrated gpu"); break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    ALOGI("\ttype                     : discrete gpu"); break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    ALOGI("\ttype                     : virtual gpu"); break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    ALOGI("\ttype                     : cpu"); break;
                default:
                    ALOGI("\ttype                     : unknown"); break;
            }

            switch ( this->m_properties.vendorID ) {
                case 0x1002:
                    ALOGI("\tvendor                   : AMD"); break;
                case 0x1010:
                    ALOGI("\tvendor                   : ImgTec"); break;
                case 0x10DE:
                    ALOGI("\tvendor                   : NVIDIA"); break;
                case 0x13B5:
                    ALOGI("\tvendor                   : ARM"); break;
                case 0x5143:
                    ALOGI("\tvendor                   : Qualcomm"); break;
                case 0x8086:
                    ALOGI("\tvendor                   : INTEL"); break;
                default:
                    ALOGI("\tvendor                   : unknown"); break;
            }

            ALOGI("\tmax memory alloc count   : %d", this->m_properties.limits.maxMemoryAllocationCount);
            ALOGI("\tmax sampler alloc count  : %d", this->m_properties.limits.maxSamplerAllocationCount);
            ALOGI("\tmax image 2d dimension   : %d", this->m_properties.limits.maxImageDimension2D);
            ALOGI("\tmax image cube dimension : %d", this->m_properties.limits.maxImageDimensionCube);
            ALOGI("\tASTC compression support : %d", this->m_features.textureCompressionASTC_LDR);
            ALOGI("\tETC2 compression support : %d", this->m_features.textureCompressionETC2);
            ALOGI("\tBC compression support   : %d", this->m_features.textureCompressionBC);
            ALOGI("\tscore                    : %d", this->m_score);
        }

    private:
        bool is_usable() const {
            // Application can't function without geometry shaders
            if ( !this->m_features.geometryShader )
                return false;

            if ( !this->m_features.samplerAnisotropy )
                return false;

            if ( !::findQueueFamilies(this->m_phys_device, this->m_surface).isComplete() )
                return false;

            if ( !this->does_support_all_extensions(::DEVICE_EXTENSIONS.begin(), ::DEVICE_EXTENSIONS.end()) )
                return false;

            const auto swapChainSupport = ::querySwapChainSupport(this->m_surface, this->m_phys_device);
            const auto swapChainAdequate = !(swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty());
            if ( !swapChainAdequate )
                return false;

            return true;
        }

        unsigned calc_score() const {
            if (!this->is_usable()) {
                return 0;
            }

            unsigned score = 0;
            {
                // Discrete GPUs have a significant performance advantage
                if ( VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == this->m_properties.deviceType )
                    score += 5000;

                // Maximum possible size of textures affects graphics quality
                score += this->m_properties.limits.maxImageDimension2D;

                if ( this->features().textureCompressionASTC_LDR ) {
                    score += 1000;
                }
            }

            return score;
        }

        template <typename _Iter>
        bool does_support_all_extensions(const _Iter begin, const _Iter end) const {
            return 0 == this->how_many_extensions_not_supported(begin, end);
        }

        template <typename _Iter>
        size_t how_many_extensions_not_supported(const _Iter begin, const _Iter end) const {
            std::set<std::string> requiredExtensions(begin, end);

            for ( const auto& extension : this->m_available_extensions ) {
                requiredExtensions.erase(extension.extensionName);
            }

            return requiredExtensions.size();
        }

    };

}


namespace {

    void init(android_app* const state) {
        if (0 == InitVulkan()) {
            __android_log_print(ANDROID_LOG_ERROR, "vulkan_practice", "failed to init vulkan");
            return;
        }

        VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .apiVersion = VK_MAKE_VERSION(1, 0, 0),
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .pApplicationName = "tutorial01_load_vulkan",
            .pEngineName = "tutorial",
        };

        // prepare necessary extensions: Vulkan on Android need these to function
        std::vector<const char*> instanceExt, deviceExt;
        instanceExt.push_back("VK_KHR_surface");
        instanceExt.push_back("VK_KHR_android_surface");
        deviceExt.push_back("VK_KHR_swapchain");

        // Create the Vulkan instance
        VkInstanceCreateInfo instanceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .pApplicationInfo = &appInfo,
            .enabledExtensionCount = static_cast<uint32_t>(instanceExt.size()),
            .ppEnabledExtensionNames = instanceExt.data(),
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
        };

        VkInstance inst = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateInstance(&instanceCreateInfo, nullptr, &inst)) {
            __android_log_print(
                ANDROID_LOG_INFO,
                "vulkan_practice",
                "failed to create vulkan instance"
            );
            return;
        }

        // if we create a surface, we need the surface extension
        VkAndroidSurfaceCreateInfoKHR createInfo{
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .window = state->window,
        };

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateAndroidSurfaceKHR(inst, &createInfo, nullptr, &surface)) {
            __android_log_print(
                ANDROID_LOG_ERROR,
                "vulkan_practice",
                "failed to create vulkan surface"
            );
            return;
        }

        uint32_t gpuCount = 0;
        if (VK_SUCCESS != vkEnumeratePhysicalDevices(inst, &gpuCount, nullptr)) {
            __android_log_print(
                ANDROID_LOG_ERROR,
                "vulkan_practice",
                "failed to get gpu count"
            );
            return;
        }
        std::vector<VkPhysicalDevice> gpu_list(gpuCount);
        if (VK_SUCCESS != vkEnumeratePhysicalDevices(inst, &gpuCount, gpu_list.data())) {
            __android_log_print(
                ANDROID_LOG_ERROR,
                "vulkan_practice",
                "failed to get gpu list"
            );
            return;
        }

        __android_log_print(
            ANDROID_LOG_INFO,
            "vulkan_practice",
            "GPU count: %d",
            gpuCount
        );
        for ( auto device : gpu_list ) {
            const ::PhysDeviceProps info{ device, surface };
            info.print_info();
        }
    }

}


extern "C" {

    void handle_cmd(android_app* const state, const int32_t cmd) {
        switch (cmd) {
            case APP_CMD_INIT_WINDOW:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: init window");
                ::init(state);
                break;
            case APP_CMD_WINDOW_RESIZED:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: init window");
                break;
            case APP_CMD_DESTROY:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: destroy");
                break;
            case APP_CMD_PAUSE:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: pause");
                break;
            case APP_CMD_RESUME:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: resume");
                break;
            case APP_CMD_START:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: start");
                break;
            case APP_CMD_STOP:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: stop");
                break;
            case APP_CMD_GAINED_FOCUS:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: focus gained");
                break;
            case APP_CMD_LOST_FOCUS:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: focus lost");
                break;
            case APP_CMD_INPUT_CHANGED:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: input changed");
                break;
            case APP_CMD_TERM_WINDOW:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: window terminated");
                break;
            case APP_CMD_SAVE_STATE:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: save state");
                break;
            default:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd; unknown (%d)", cmd);
                break;
        }
    }

    void android_main(struct android_app *pApp) {
        pApp->onAppCmd = handle_cmd;

        int events;
        android_poll_source *pSource;
        do {
            if (ALooper_pollAll(0, nullptr, &events, (void **) &pSource) >= 0) {
                if (pSource) {
                    pSource->process(pApp, pSource);
                }
            }
        } while (!pApp->destroyRequested);
    }

}
