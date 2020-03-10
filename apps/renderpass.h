#pragma once

#include <vulkan/vulkan.h>


namespace dal {

    class RenderPass {

    private:
        VkRenderPass m_renderPass;

    public:
        void init(VkDevice logiDevice, const VkFormat swapChainImageFormat);
        void destroy(VkDevice logiDevice);

    };

}
