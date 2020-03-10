#pragma once

#include <vulkan/vulkan.h>


namespace dal {

    class PhysDevice {

    private:
        VkPhysicalDevice m_handle = VK_NULL_HANDLE;

    public:
        void init(VkInstance instance, VkSurfaceKHR surface);
        // Physical device is destoryed implicitly when the corresponding VkInstance is destroyed.

        auto get(void) const {
            return this->m_handle;
        }

    };

}
