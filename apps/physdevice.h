#pragma once

#include <vulkan/vulkan.h>


namespace dal {

    class PhysDeviceProps {

    private:
        VkPhysicalDevice m_phys_device;
        VkSurfaceKHR m_surface;

        VkPhysicalDeviceProperties m_properties;
        VkPhysicalDeviceFeatures m_features;
        std::vector<VkExtensionProperties> m_available_extensions;

        uint32_t m_score = 0;

    public:
        PhysDeviceProps(const VkPhysicalDevice physDevice, const VkSurfaceKHR surface);

        uint32_t score() const;
        void print_info() const;

    private:
        bool is_usable() const;
        unsigned calc_score() const;

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


    class PhysDevice {

    private:
        VkPhysicalDevice m_handle = VK_NULL_HANDLE;
        PhysDeviceProps m_info;

    public:
        void init(VkInstance instance, VkSurfaceKHR surface);
        // Physical device is destoryed implicitly when the corresponding VkInstance is destroyed.

        auto get(void) const {
            return this->m_handle;
        }

    };

}
