#pragma once

#include <vulkan/vulkan.h>


namespace dal {

    class ShaderPipeline {

    private:
        VkPipelineLayout m_pipelineLayout;

    public:
        void init(VkDevice device, const VkExtent2D& extent);
        void destroy(VkDevice device);

    };

}
