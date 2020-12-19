#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "physdevice.h"
#include "logidevice.h"
#include "swapchain.h"
#include "swapchain_images.h"
#include "shader.h"
#include "renderpass.h"
#include "fbufmanager.h"
#include "command_pool.h"
#include "vkommand.h"
#include "semaphore.h"
#include "vert_data.h"
#include "uniform.h"
#include "texture.h"


namespace dal {

    class VulkanMaster {

    private:
        PhysDevice m_physDevice;
        LogiDeviceAndQueue m_logiDevice;
        Swapchain m_swapchain;
        SwapchainImages m_swapchainImages;
        RenderPass m_renderPass;
        ShaderPipeline m_pipeline;
        FbufManager m_fbuf;
        CommandPool m_cmdPool;
        CommandBuffers m_cmdBuffers;
        SyncMaster m_syncMas;
        DescriptorSetLayout m_descSetLayout;
        UniformBuffers m_uniformBufs;
        DescriptorPool m_descPool;

        TextureImage m_tex1;
        std::vector<MeshBuffer> m_meshes;

        unsigned m_currentFrame = 0;
        bool m_needResize = false;
        unsigned m_scrWidth, m_scrHeight;

    public:
        void init(const VkInstance instance, const VkSurfaceKHR surface, const unsigned w, const unsigned h);
        void destroy(void);

        void render(const VkSurfaceKHR surface);
        void waitLogiDeviceIdle(void) const;
        void recreateSwapChain(const VkSurfaceKHR surface);

        void notifyScreenResize(const unsigned w, const unsigned h);

    private:
        void initSwapChain(const VkSurfaceKHR surface);
        void destroySwapChain();

    };

}
