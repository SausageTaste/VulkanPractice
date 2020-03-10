#pragma once

#include <array>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vkdevice.h"


namespace dal {

    class VulkanWindowGLFW {

    private:
        using extList_t = std::vector<const char*>;

    private:
        GLFWwindow* m_window = nullptr;
        VkInstance m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VulkanMaster m_device;

#ifndef NDEBUG
        VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
#endif

    public:
        VulkanWindowGLFW(void);
        ~VulkanWindowGLFW(void);

        void update(void);
        bool isOughtToClose(void);

    };

}
