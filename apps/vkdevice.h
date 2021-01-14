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
#include "depth_image.h"
#include "model_render.h"
#include "view_camera.h"


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
        UniformBuffer_PerFrame m_ubuf_per_frame_in_composition;
        DescriptorPool m_descPool;
        DepthImage m_depth_image;
        GbufManager m_gbuf;
        TextureManager m_tex_man;

        U_PerFrame_InComposition m_data_per_frame_in_composition;
        CameraLookAt m_camera;
        std::vector<ModelVK> m_models;
        std::shared_ptr<TextureUnit> m_tex_grass, m_tex_tile;

        unsigned m_currentFrame = 0;
        bool m_needResize = false;
        unsigned m_scrWidth, m_scrHeight;

    public:
        ~VulkanMaster() {
            this->destroy();
        }

        void init(const VkInstance instance, const VkSurfaceKHR surface, const unsigned w, const unsigned h);
        void destroy(void);

        auto& camera() {
            return this->m_camera;
        }

        void render(const VkSurfaceKHR surface);
        void waitLogiDeviceIdle(void) const;
        void recreateSwapChain(const VkSurfaceKHR surface);

        void load_textures();
        void load_models();

        void notifyScreenResize(const unsigned w, const unsigned h);

    private:
        void initSwapChain(const VkSurfaceKHR surface);
        void destroySwapChain();
        auto make_attachment_format_array() const {
            return this->m_gbuf.make_formats_array(this->m_swapchain.imageFormat(), this->m_depth_image.format());
        }

    };

}
