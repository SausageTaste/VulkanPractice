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
        DescriptorSetManager m_desc_man;
        DepthImage m_depth_image;
        GbufManager m_gbuf;
        TextureManager m_tex_man;

        UniformBufferArray<U_PerFrame_InDeferred> m_ubuf_per_frame_in_deferred;
        UniformBufferArray<U_PerFrame_InComposition> m_ubuf_per_frame_in_composition;

        Scene m_scene;
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
            return this->m_scene.m_camera;
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
        void submit_render_to_shadow_maps(const uint32_t swapchain_index);
        void udpate_uniform_buffers(const uint32_t swapchain_index);

        auto make_attachment_format_array() const {
            return this->m_gbuf.make_formats_array(this->m_swapchain.imageFormat(), this->m_depth_image.format());
        }

    };

}
