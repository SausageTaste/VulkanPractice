#pragma once

#include <vulkan/vulkan.h>


namespace dal {

    class RenderPass {

    private:
        VkRenderPass m_renderPass;

    public:
        void init(VkDevice logiDevice, const VkFormat swapChainImageFormat, const VkFormat depth_format);
        void destroy(VkDevice logiDevice);

        auto get(void) const {
            return this->m_renderPass;
        }

    };

}
