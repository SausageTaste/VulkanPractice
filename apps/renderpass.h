#pragma once

#include <array>

#include <vulkan/vulkan.h>


namespace dal {

    class RenderPass {

    private:
        VkRenderPass m_rendering_rp = VK_NULL_HANDLE;
        VkRenderPass m_shadow_map_rp = VK_NULL_HANDLE;

    public:
        void init(const VkDevice logiDevice, const std::array<VkFormat, 6>& attachment_formats);
        void destroy(VkDevice logiDevice);

        auto get(void) const {
            return this->m_rendering_rp;
        }
        auto shadow_mapping() const {
            return this->m_shadow_map_rp;
        }

    };

}
