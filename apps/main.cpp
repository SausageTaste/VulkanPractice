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

    static VkInstanceCreateInfo makeInstCreateInfo(const VkApplicationInfo& appInfo) {
        VkInstanceCreateInfo createInfo = {};

        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        {
            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            createInfo.enabledExtensionCount = glfwExtensionCount;
            createInfo.ppEnabledExtensionNames = glfwExtensions;
        }

        createInfo.enabledLayerCount = 0;

        return createInfo;
    }

    static void printVulkanExtensions(void) {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        std::cout << "available extensions are following\n";
        for ( const auto& ext : extensions ) {
            std::cout << "    " << ext.extensionName << '\n';
        }
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
