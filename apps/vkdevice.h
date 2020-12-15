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
#include "vkommand.h"
#include "semaphore.h"


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
        CommandPool m_command;
        SyncMaster m_syncMas;

        unsigned m_currentFrame = 0;

    public:
        void init(const VkInstance instance, const VkSurfaceKHR surface);
        void destroy(void);

        void render(void);
        void waitLogiDeviceIdle(void) const;

    private:
        void initSwapChain(const VkSurfaceKHR surface);
        void destroySwapChain();

    };

}
