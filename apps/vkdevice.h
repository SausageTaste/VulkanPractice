#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "swapchain_images.h"
#include "shader.h"
#include "renderpass.h"
#include "fbufmanager.h"


namespace dal {

    class Uncopiable {

    public:
        Uncopiable(void) = default;
        Uncopiable(const Uncopiable&) = delete;
        Uncopiable& operator=(const Uncopiable&) = delete;

    };


    class PhysDevice : public Uncopiable {

    private:
        VkPhysicalDevice m_handle = VK_NULL_HANDLE;

    public:
        void init(VkInstance instance, VkSurfaceKHR surface);
        // Physical device is destoryed implicitly when the corresponding VkInstance is destroyed.

        auto get(void) const {
            return this->m_handle;
        }

    private:
        static unsigned rateDeviceSuitability(VkSurfaceKHR surface, VkPhysicalDevice physDevice);
        static bool checkDeviceExtensionSupport(VkPhysicalDevice physDevice);
        static void printDeviceInfo(const VkPhysicalDeviceProperties& properties, const VkPhysicalDeviceFeatures& features);

    };


    class LogiDeviceAndQueue : public Uncopiable {

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


    class Swapchain {

    private:
        VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;

        VkFormat m_imageFormat;
        VkExtent2D m_extent;

    public:
        void init(VkSurfaceKHR surface, VkPhysicalDevice physDevice, VkDevice logiDevice);
        void destroy(VkDevice logiDevice);

        VkSwapchainKHR get(void) {
            return this->m_swapChain;
        }
        VkFormat imageFormat(void) const {
            return this->m_imageFormat;
        }
        VkExtent2D extent(void) const {
            return this->m_extent;
        }

    private:
        static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    };


    class GraphicDevice {

    private:
        PhysDevice m_physDevice;
        LogiDeviceAndQueue m_logiDevice;
        Swapchain m_swapchain;
        SwapchainImages m_swapchainImages;
        RenderPass m_renderPass;
        ShaderPipeline m_pipeline;
        FbufManager m_fbuf;

    public:
        void init(const VkInstance instance, const VkSurfaceKHR surface);
        void destroy(void);

    };

}
