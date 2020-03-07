#include "vkwindow.h"

#include <iostream>
#include <exception>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>


namespace {

    const char* const WINDOW_TITLE = "Vulkan Practice";

    const std::array<const char*, 1> VAL_LAYERS_TO_USE = {
        "VK_LAYER_KHRONOS_validation"
    };

}


namespace dal {

    VulkanWindowGLFW::VulkanWindowGLFW(const int width, const int height) {
        {
            glfwInit();
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

            this->m_window = glfwCreateWindow(width, height, WINDOW_TITLE, nullptr, nullptr);
            if ( nullptr == this->m_window ) {
                throw std::runtime_error{ "Failed to create window." };
            }
        }

        {
            const auto appInfo = this->makeAppInfo();
            auto createInfo = this->makeInstCreateInfo(appInfo);

            const auto extensions = this->getRequiredExtensions();
            createInfo.enabledExtensionCount = extensions.size();
            createInfo.ppEnabledExtensionNames = extensions.data();

            if ( VK_SUCCESS != vkCreateInstance(&createInfo, nullptr, &this->m_instance) ) {
                throw std::runtime_error{ "Failed to create vulkan instance. " };
            }
        }
    }

    VulkanWindowGLFW::~VulkanWindowGLFW(void) {
        vkDestroyInstance(this->m_instance, nullptr);
        this->m_instance = nullptr;

        glfwDestroyWindow(this->m_window);
        this->m_window = nullptr;
        glfwTerminate();
    }

    void VulkanWindowGLFW::update(void) {
        glfwPollEvents();
    }

    bool VulkanWindowGLFW::isOughtToClose(void) {
        return glfwWindowShouldClose(this->m_window);
    }

    // Private

    VkInstanceCreateInfo VulkanWindowGLFW::makeInstCreateInfo(const VkApplicationInfo& appInfo) const {
        VkInstanceCreateInfo createInfo = {};

        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        {
#ifdef NDEBUG
            createInfo.enabledLayerCount = 0;
#else
            if ( !this->checkValidationLayerSupport() ) {
                throw std::runtime_error{ "validation layers requested, but not available!" };
            }

            createInfo.enabledLayerCount = static_cast<uint32_t>(VAL_LAYERS_TO_USE.size());
            createInfo.ppEnabledLayerNames = VAL_LAYERS_TO_USE.data();
#endif
        }

        return createInfo;
    }

    std::vector<const char*> VulkanWindowGLFW::getRequiredExtensions(void) const {
        uint32_t glfwExtCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);

        std::vector<const char*> extensions{ glfwExtensions, glfwExtensions + glfwExtCount };

#ifndef NDEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        return extensions;
    }

    // Private static

    VkApplicationInfo VulkanWindowGLFW::makeAppInfo(void) {
        VkApplicationInfo appInfo = {};

        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan Practice";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        return appInfo;
    }

    bool VulkanWindowGLFW::checkValidationLayerSupport(void) {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for ( const char* layerName : VAL_LAYERS_TO_USE ) {
            bool layerFound = false;

            for ( const auto& layerProperties : availableLayers ) {
                if ( strcmp(layerName, layerProperties.layerName) == 0 ) {
                    layerFound = true;
                    break;
                }
            }

            if ( !layerFound )
                return false;
        }

        return true;
    }

}
