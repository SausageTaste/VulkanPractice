#include "vkwindow.h"

#include <iostream>
#include <exception>

#include "konst.h"
#include "timer.h"


namespace {

    const char* const WINDOW_TITLE = "Vulkan Practice";

    dal::Timer g_timer, g_timer_for_delta_time;
    size_t g_fpsCounter = 0;

}


namespace {

    class {

    public:
        bool m_left = false;
        bool m_right = false;
        bool m_forward = false;
        bool m_back = false;

        bool m_up = false;
        bool m_down = false;

        bool m_look_left = false;
        bool m_look_right = false;
        bool m_look_up = false;
        bool m_look_down = false;

    public:
        auto make_move_direc() const {
            glm::vec3 result{ 0, 0, 0 };

            if (this->m_left) {
                result.x -= 1;
            }
            if (this->m_right) {
                result.x += 1;
            }
            if (this->m_forward) {
                result.z -= 1;
            }
            if (this->m_back) {
                result.z += 1;
            }

            if (this->m_up) {
                result.y += 1;
            }
            if (this->m_down) {
                result.y -= 1;
            }

            return result;
        }

        auto make_rotation_angles() const {
            glm::vec2 result{ 0, 0 };

            if (this->m_look_left) {
                result.y += 1;
            }
            if (this->m_look_right) {
                result.y -= 1;
            }

            if (this->m_look_up) {
                result.x += 1;
            }
            if (this->m_look_down) {
                result.x -= 1;
            }

            return result;
        }

    } g_input_state;

    void callback_keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods) {
        bool flag_is_down;

        switch ( action ) {
        case GLFW_PRESS:
        case GLFW_REPEAT:
            flag_is_down = true;
            break;
        case GLFW_RELEASE:
            flag_is_down = false;
            break;
        default:
            return;
        }

        switch (key) {
        case GLFW_KEY_W:
            g_input_state.m_forward = flag_is_down;
            break;
        case GLFW_KEY_A:
            g_input_state.m_left = flag_is_down;
            break;
        case GLFW_KEY_S:
            g_input_state.m_back = flag_is_down;
            break;
        case GLFW_KEY_D:
            g_input_state.m_right = flag_is_down;
            break;

        case GLFW_KEY_SPACE:
            g_input_state.m_up = flag_is_down;
            break;
        case GLFW_KEY_LEFT_CONTROL:
            g_input_state.m_down = flag_is_down;
            break;

        case GLFW_KEY_LEFT:
            g_input_state.m_look_left = flag_is_down;
            break;
        case GLFW_KEY_RIGHT:
            g_input_state.m_look_right = flag_is_down;
            break;
        case GLFW_KEY_UP:
            g_input_state.m_look_up = flag_is_down;
            break;
        case GLFW_KEY_DOWN:
            g_input_state.m_look_down = flag_is_down;
            break;

        default:
            return;
        }
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<dal::VulkanWindowGLFW*>(glfwGetWindowUserPointer(window));
        app->onResize(width, height);
    }

    GLFWwindow* createWindowGLFW(const unsigned width, const unsigned height, const char* const title) {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        auto window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if ( nullptr == window )
            throw std::runtime_error{ "Failed to create window." };

        glfwSetKeyCallback(window, ::callback_keyEvent);
        glfwSetFramebufferSizeCallback(window, ::framebufferResizeCallback);

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
            return VK_FALSE;
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

        const auto err_msg = std::string{ "Vulkan Debug: " } + pCallbackData->pMessage;
        std::cerr << err_msg << std::endl;
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
        glfwSetWindowUserPointer(this->m_window, this);

        createVulkanInstance(this->m_instance);
#ifndef NDEBUG
        setupDebugMessenger(this->m_instance, this->m_debugMessenger);
#endif
        this->m_surface = createSurface(this->m_instance, this->m_window);
        this->m_device.init(this->m_instance, this->m_surface, WIN_WIDTH, WIN_HEIGHT);

        g_timer.check();
        g_timer_for_delta_time.check();
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
        const auto delta_time = g_timer_for_delta_time.checkGetElapsed();

        // Print FPS
        {
            if ( g_timer.getElapsed() > 1.0 ) {
                std::cout << "FPS: " << g_fpsCounter << std::endl;
                g_timer.check();
                g_fpsCounter = 0;
            }
            else {
                g_fpsCounter++;
            }
        }

        // Input
        {
            glfwPollEvents();

            constexpr float MOVE_SPEED = 2;

            const auto move_vec = g_input_state.make_move_direc();
            this->m_device.camera().move_horizontal(MOVE_SPEED * move_vec.x * delta_time, MOVE_SPEED * move_vec.z * delta_time);
            this->m_device.camera().m_pos.y += MOVE_SPEED * move_vec.y * delta_time;

            const auto rotation_angles = g_input_state.make_rotation_angles();
            this->m_device.camera().m_rotations.x += rotation_angles.x * delta_time;
            this->m_device.camera().m_rotations.y += rotation_angles.y * delta_time;
        }

        this->m_device.render(this->m_surface);
    }

    bool VulkanWindowGLFW::isOughtToClose(void) {
        return glfwWindowShouldClose(this->m_window);
    }

    void VulkanWindowGLFW::waitSafeExit(void) {
        this->m_device.waitLogiDeviceIdle();
    }

    void VulkanWindowGLFW::onResize(const unsigned w, const unsigned h) {
        int width = 0, height = 0;
        glfwGetFramebufferSize(this->m_window, &width, &height);
        while (width == 0 || height == 0) {
            printf("wait\n");
            glfwGetFramebufferSize(this->m_window, &width, &height);
            glfwWaitEvents();
        }

        this->m_device.notifyScreenResize(w, h);
    }

}
