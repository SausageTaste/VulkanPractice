#include <exception>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>


class Window {

private:
    const char* const WINDOW_TITLE = "Vulkan Practice";

    GLFWwindow* m_window = nullptr;

public:
    Window(const int width, const int height) {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        this->m_window = glfwCreateWindow(width, height, this->WINDOW_TITLE, nullptr, nullptr);
        if (nullptr == this->m_window) {
            throw std::runtime_error{ "Failed to create window." };
        }

        {
            uint32_t extensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
            std::cout << extensionCount << " extensions supported" << std::endl;
        }
    }

    ~Window(void) {
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

};


class VulkanInstance {

private:
    VkInstance m_instance;

public:
    VulkanInstance(void) {
        VkApplicationInfo appInfo;
        {
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "Vulkan Practice";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "No Engine";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_0;
        }

        VkInstanceCreateInfo createInfo;
        {
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;

            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            createInfo.enabledExtensionCount = glfwExtensionCount;
            createInfo.ppEnabledExtensionNames = glfwExtensions;

            createInfo.enabledLayerCount = 0;
        }

        if ( VK_SUCCESS != vkCreateInstance(&createInfo, nullptr, &this->m_instance) ) {
            throw std::runtime_error{ "Failed to create vulkan instance. "};
        }
    }

}


int main() {
    Window window{ 800, 450 };

    while ( !window.isOughtToClose() ) {
        window.update();
    }

    return 0;
}
