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

    class PhysDeviceProps {

    private:
        VkPhysicalDevice m_phys_device;
        VkSurfaceKHR m_surface;

        VkPhysicalDeviceProperties m_properties;
        VkPhysicalDeviceFeatures m_features;
        std::vector<VkExtensionProperties> m_available_extensions;

        uint32_t m_score = 0;

    public:
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

        auto& score() const {
            return this->m_score;
        }

        bool is_usable() const {
            // Application can't function without geometry shaders
            if ( !this->m_features.geometryShader )
                return false;

            if ( !this->m_features.samplerAnisotropy )
                return false;

            if ( !this->m_features.textureCompressionASTC_LDR )
                return false;

            if ( !dal::findQueueFamilies(this->m_phys_device, this->m_surface).isComplete() )
                return false;

            if ( !this->does_support_all_extensions(dal::DEVICE_EXTENSIONS.begin(), dal::DEVICE_EXTENSIONS.end()) )
                return false;

            const auto swapChainSupport = dal::querySwapChainSupport(this->m_surface, this->m_phys_device);
            const auto swapChainAdequate = !(swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty());
            if ( !swapChainAdequate )
                return false;

            return true;
        }

        void print_info() const {
            std::cout << "physical device \"" << this->m_properties.deviceName << "\"\n";

            std::cout << "\ttype                     : ";
            switch ( this->m_properties.deviceType ) {

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

            std::cout << "\tvendor                   : ";
            switch ( this->m_properties.vendorID ) {

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

            std::cout << "\tmax memory alloc count   : " << this->m_properties.limits.maxMemoryAllocationCount << '\n';
            std::cout << "\tmax sampler alloc count  : " << this->m_properties.limits.maxSamplerAllocationCount << '\n';
            std::cout << "\tmax image 2d dimension   : " << this->m_properties.limits.maxImageDimension2D << '\n';
            std::cout << "\tmax image cube dimension : " << this->m_properties.limits.maxImageDimensionCube << '\n';
            std::cout << "\tASTC compression support : " << this->m_features.textureCompressionASTC_LDR << '\n';
            std::cout << "\tETC2 compression support : " << this->m_features.textureCompressionETC2 << '\n';
            std::cout << "\tBC compression support   : " << this->m_features.textureCompressionBC << '\n';
            std::cout << "\tscore                    : " << this->m_score << '\n';
        }

    private:
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
            }

            return score;
        }

        template <typename _Iter>
        bool does_support_all_extensions(const _Iter begin, const _Iter end) const {
            std::set<std::string> requiredExtensions(begin, end);

            for ( const auto& extension : this->m_available_extensions ) {
                requiredExtensions.erase(extension.extensionName);
            }

            return requiredExtensions.empty();
        }

        /*
        bool does_support_astc_texture() const {
            const std::array<VkFormat, 1> NEEDED_FORMATE = {
                VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
            };

            for (const auto format : NEEDED_FORMATE) {
                VkFormatProperties properties;
                vkGetPhysicalDeviceFormatProperties(this->m_phys_device, format, &properties);
                bool supportsASTC = (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;
                if (!supportsASTC) {
                    return false;
                }
            }

            return true;
        }
        */

    };

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

        unsigned bestScore = 0;

        for ( auto device : devices ) {
            const ::PhysDeviceProps info{ device, surface };
#if DAL_PRINT_DEVICE_INFO
            info.print_info();
#endif
            if ( info.score() > bestScore ) {
                this->m_handle = device;
                bestScore = info.score();
            }
        }

        if ( VK_NULL_HANDLE == this->m_handle ) {
            throw std::runtime_error{ "failed to find a sutable graphic device." };
        }

#if DAL_PRINT_DEVICE_INFO
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(this->m_handle, &properties);
        std::cout << "Selected GPU: \"" << properties.deviceName << "\"\n";
#endif

    }

}
