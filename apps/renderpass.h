#pragma once

#include <array>

#include <vulkan/vulkan.h>


namespace dal {

    class RenderPass {

    private:
        VkRenderPass m_renderPass;

    public:
        void init(const VkDevice logiDevice, const std::array<VkFormat, 6>& attachment_formats);
        void destroy(VkDevice logiDevice);

        auto get(void) const {
            return this->m_renderPass;
        }

    };

}
