#include <array>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


namespace dal {

    class VulkanWindowGLFW {

    private:
        using extList_t = std::vector<const char*>;

    private:
        GLFWwindow* m_window = nullptr;
        VkInstance m_instance = nullptr;

#ifndef NDEBUG
        VkDebugUtilsMessengerEXT m_debugMessenger = nullptr;
#endif

    public:
        VulkanWindowGLFW(const unsigned width, const unsigned height);
        ~VulkanWindowGLFW(void);

        void update(void);
        bool isOughtToClose(void);

    };

}
