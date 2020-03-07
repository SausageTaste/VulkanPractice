#include <array>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


namespace dal {

    class VulkanWindowGLFW {

    private:
        GLFWwindow* m_window = nullptr;
        VkInstance m_instance = nullptr;

    public:
        VulkanWindowGLFW(const int width, const int height);
        ~VulkanWindowGLFW(void);

        void update(void);
        bool isOughtToClose(void);

    private:
        VkInstanceCreateInfo makeInstCreateInfo(const VkApplicationInfo& appInfo) const;
        std::vector<const char*> getRequiredExtensions(void) const;

        static VkApplicationInfo makeAppInfo(void);
        static bool checkValidationLayerSupport(void);

    };

}
