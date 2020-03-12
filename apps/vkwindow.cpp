#include "vkwindow.h"

#include <iostream>
#include <exception>

#include "konst.h"

/*
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
*/


namespace {

    const char* const WINDOW_TITLE = "Vulkan Practice";

}


namespace {

    GLFWwindow* createWindowGLFW(const unsigned width, const unsigned height, const char* const title) {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        auto window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if ( nullptr == window )
            throw std::runtime_error{ "Failed to create window." };

        return window;
    }

}


// Vulkan functions
namespace {

#ifndef NDEBUG
    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
    {
        switch ( messageSeverity ) {

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            std::cerr << "[VERB] ";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            std::cerr << "[INFO] ";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            std::cerr << "[WARN] ";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            std::cerr << "[ERRO] ";
            break;
        default:
            std::cerr << "[WTF?] ";
            break;

        }

        std::cerr << "Vulkan Debug: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if ( func != nullptr ) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if ( func != nullptr ) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    VkDebugUtilsMessengerCreateInfoEXT makeDebugMessengerCreateInfo(void) {
        VkDebugUtilsMessengerCreateInfoEXT createInfo = {};

        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;

        return createInfo;
    }

    void setupDebugMessenger(const VkInstance& instance, VkDebugUtilsMessengerEXT& debugMsger) {
        const auto createInfo = makeDebugMessengerCreateInfo();

        if ( VK_SUCCESS != createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMsger) ) {
            throw std::runtime_error{ "Failed to set up debug messenger!" };
        }
    }
#endif

    VkApplicationInfo makeVKAppInfo(void) {
        VkApplicationInfo appInfo = {};

        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = WINDOW_TITLE;
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        return appInfo;
    }

    std::vector<const char*> getRequiredExtensions(void) {
        uint32_t glfwExtCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);

        std::vector<const char*> extensions{ glfwExtensions, glfwExtensions + glfwExtCount };

#ifndef NDEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        return extensions;
    }

    bool checkValidationLayerSupport(void) {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for ( const char* layerName : dal::VAL_LAYERS_TO_USE ) {
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

    void createVulkanInstance(VkInstance& instance) {
        const auto appInfo = makeVKAppInfo();
        const auto extensions = getRequiredExtensions();
#ifndef NDEBUG
        const auto debugCreateInfo = makeDebugMessengerCreateInfo();
#endif

        VkInstanceCreateInfo createInfo = {};
        {
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;

            createInfo.enabledExtensionCount = extensions.size();
            createInfo.ppEnabledExtensionNames = extensions.data();

            {
#ifdef NDEBUG
                createInfo.enabledLayerCount = 0;
                createInfo.pNext = nullptr;
#else
                if ( !checkValidationLayerSupport() ) {
                    throw std::runtime_error{ "validation layers requested, but not available!" };
                }

                createInfo.enabledLayerCount = static_cast<uint32_t>(dal::VAL_LAYERS_TO_USE.size());
                createInfo.ppEnabledLayerNames = dal::VAL_LAYERS_TO_USE.data();

                createInfo.pNext = reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
#endif
            }
        }

        if ( VK_SUCCESS != vkCreateInstance(&createInfo, nullptr, &instance) ) {
            throw std::runtime_error{ "Failed to create vulkan instance. " };
        }
    }

    VkSurfaceKHR createSurface(const VkInstance instance, GLFWwindow* const window) {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if ( VK_SUCCESS != glfwCreateWindowSurface(instance, window, nullptr, &surface) ) {
            throw std::runtime_error{ "failed to create window surface!" };
        }
        return surface;
    }

}


namespace dal {

    VulkanWindowGLFW::VulkanWindowGLFW(void) {
        this->m_window = createWindowGLFW(WIN_WIDTH, WIN_HEIGHT, WINDOW_TITLE);
        createVulkanInstance(this->m_instance);
#ifndef NDEBUG
        setupDebugMessenger(this->m_instance, this->m_debugMessenger);
#endif
        this->m_surface = createSurface(this->m_instance, this->m_window);
        this->m_device.init(this->m_instance, this->m_surface);
    }

    VulkanWindowGLFW::~VulkanWindowGLFW(void) {
        this->m_device.destroy();

#ifndef NDEBUG
        destroyDebugUtilsMessengerEXT(this->m_instance, this->m_debugMessenger, nullptr);
        this->m_debugMessenger = VK_NULL_HANDLE;
#endif

        vkDestroySurfaceKHR(this->m_instance, this->m_surface, nullptr);
        this->m_surface = VK_NULL_HANDLE;

        vkDestroyInstance(this->m_instance, nullptr);
        this->m_instance = VK_NULL_HANDLE;

        glfwDestroyWindow(this->m_window);
        this->m_window = nullptr;
        glfwTerminate();
    }

    void VulkanWindowGLFW::update(void) {
        glfwPollEvents();
        this->m_device.render();
    }

    bool VulkanWindowGLFW::isOughtToClose(void) {
        return glfwWindowShouldClose(this->m_window);
    }

    void VulkanWindowGLFW::waitSafeExit(void) {
        this->m_device.waitLogiDeviceIdle();
    }

}
