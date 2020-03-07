#include <array>
#include <exception>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>


class VulkanWindowGLFW {

private:
    const char* const WINDOW_TITLE = "Vulkan Practice";

    const std::array<const char*, 1> VAL_LAYERS_TO_USE = {
        "VK_LAYER_KHRONOS_validation"
    };

    GLFWwindow* m_window = nullptr;
    VkInstance m_instance = nullptr;

public:
    VulkanWindowGLFW(const int width, const int height) {
        {
            glfwInit();
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

            this->m_window = glfwCreateWindow(width, height, this->WINDOW_TITLE, nullptr, nullptr);
            if ( nullptr == this->m_window ) {
                throw std::runtime_error{ "Failed to create window." };
            }
        }
       
        {
            const auto appInfo = this->makeAppInfo();
            const auto createInfo = this->makeInstCreateInfo(appInfo);

            if ( VK_SUCCESS != vkCreateInstance(&createInfo, nullptr, &this->m_instance) ) {
                throw std::runtime_error{ "Failed to create vulkan instance. " };
            }
        }
    }

    ~VulkanWindowGLFW(void) {
        vkDestroyInstance(this->m_instance, nullptr);
        this->m_instance = nullptr;

        glfwDestroyWindow(this->m_window);
        this->m_window = nullptr;
        glfwTerminate();
    }

    void update(void) {
        glfwPollEvents();
    }

    bool isOughtToClose(void) {
        return glfwWindowShouldClose(this->m_window);
    }

private:
    static VkApplicationInfo makeAppInfo(void) {
        VkApplicationInfo appInfo = {};

        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan Practice";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        return appInfo;
    }

    VkInstanceCreateInfo makeInstCreateInfo(const VkApplicationInfo& appInfo) {
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

            createInfo.enabledLayerCount = static_cast<uint32_t>(this->VAL_LAYERS_TO_USE.size());
            createInfo.ppEnabledLayerNames = this->VAL_LAYERS_TO_USE.data();
#endif
        }

        {
            const auto extensions = this->getRequiredExtensions();
            createInfo.enabledExtensionCount = extensions.size();
            createInfo.ppEnabledExtensionNames = extensions.data();
        }

        return createInfo;
    }

    std::vector<const char*> getRequiredExtensions(void) const {
        uint32_t glfwExtCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);

        std::vector<const char*> extensions{ glfwExtensions, glfwExtensions + glfwExtCount };

#ifndef NDEBUG
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        return extensions;
    }

    bool checkValidationLayerSupport(void) const {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for ( const char* layerName : this->VAL_LAYERS_TO_USE ) {
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

};


int main(int argc, char* argv) {
    try {
        VulkanWindowGLFW window{ 800, 450 };

        while ( !window.isOughtToClose() ) {
            window.update();
        }
    }
    catch ( const std::exception & e ) {
        std::cout << "Fatal Exception: " << e.what() << std::endl;
    }

    return 0;
}
