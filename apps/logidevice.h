#pragma once

#include <vulkan/vulkan.h>


namespace dal {

    class LogiDeviceAndQueue {

    private:
        VkDevice m_logiDevice = VK_NULL_HANDLE;
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_presentQueue = VK_NULL_HANDLE;

    public:
        void init(VkSurfaceKHR surface, VkPhysicalDevice physDevice);
        void destroy(void);

        auto get(void) const {
            return this->m_logiDevice;
        }

    };

}
