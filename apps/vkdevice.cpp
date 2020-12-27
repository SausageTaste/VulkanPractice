#include "vkdevice.h"

#include <array>
#include <stdexcept>

#include "util_windows.h"


namespace {

    bool isResizeNeeded(const VkResult res) {
        if (VK_ERROR_OUT_OF_DATE_KHR  == res) {
            return true;
        }
        else if (VK_SUBOPTIMAL_KHR == res) {
            return true;
        }
        else {
            return false;
        }
    }

}


// VulkanMaster
namespace dal {

    void VulkanMaster::init(const VkInstance instance, const VkSurfaceKHR surface, const unsigned w, const unsigned h) {
        this->m_physDevice.init(instance, surface);
        this->m_logiDevice.init(surface, this->m_physDevice.get());
        this->m_swapchain.init(surface, this->m_physDevice.get(), this->m_logiDevice.get(), this->m_scrWidth, this->m_scrHeight);
        this->m_swapchainImages.init(this->m_logiDevice.get(), this->m_swapchain.get(), this->m_swapchain.imageFormat(), this->m_swapchain.extent());
        this->m_depth_image.init(this->m_swapchain.extent(), this->m_logiDevice.get(), this->m_physDevice.get());
        this->m_renderPass.init(this->m_logiDevice.get(), this->m_swapchain.imageFormat(), this->m_depth_image.format());
        this->m_descSetLayout.init(this->m_logiDevice.get());
        this->m_pipeline.init(this->m_logiDevice.get(), this->m_renderPass.get(), this->m_swapchain.extent(), this->m_descSetLayout.get());
        this->m_fbuf.init(this->m_logiDevice.get(), this->m_renderPass.get(), this->m_swapchainImages.getViews(), this->m_swapchain.extent(), this->m_depth_image.image_view());
        this->m_cmdPool.init(this->m_physDevice.get(), this->m_logiDevice.get(), surface);

        this->m_sampler1.init(this->m_logiDevice.get(), this->m_physDevice.get());
        {
            this->m_textures.emplace_back();
            if (this->m_physDevice.does_support_astc()) {
                this->m_textures.back().image.init_astc(
                    (dal::findResPath() + "/image/grass1.astc").c_str(),
                    this->m_logiDevice.get(), this->m_physDevice.get(),
                    this->m_cmdPool,
                    this->m_logiDevice.graphicsQ()
                );
            }
            else {
                this->m_textures.back().image.init_img(
                    (dal::findResPath() + "/image/grass1.png").c_str(),
                    this->m_logiDevice.get(), this->m_physDevice.get(),
                    this->m_cmdPool,
                    this->m_logiDevice.graphicsQ()
                );
            }
            this->m_textures.back().view.init(
                this->m_logiDevice.get(),
                this->m_textures.back().image.image(),
                this->m_textures.back().image.format(),
                this->m_textures.back().image.mip_level()
            );

            this->m_textures.emplace_back();
            if (this->m_physDevice.does_support_astc()) {
                this->m_textures.back().image.init_astc(
                    (dal::findResPath() + "/image/0021di.astc").c_str(),
                    this->m_logiDevice.get(), this->m_physDevice.get(),
                    this->m_cmdPool,
                    this->m_logiDevice.graphicsQ()
                );
            }
            else {
                this->m_textures.back().image.init_img(
                    (dal::findResPath() + "/image/0021di.png").c_str(),
                    this->m_logiDevice.get(), this->m_physDevice.get(),
                    this->m_cmdPool,
                    this->m_logiDevice.graphicsQ()
                );
            }
            this->m_textures.back().view.init(
                this->m_logiDevice.get(),
                this->m_textures.back().image.image(),
                this->m_textures.back().image.format(),
                this->m_textures.back().image.mip_level()
            );
        }

        this->m_uniformBufs.init(this->m_logiDevice.get(), this->m_physDevice.get(), this->m_swapchainImages.size());
        this->m_descPool.initPool(this->m_logiDevice.get(), this->m_swapchainImages.size());
        for (const auto& tex : this->m_textures) {
            this->m_descPool.addSets(
                this->m_logiDevice.get(), this->m_swapchainImages.size(), this->m_descSetLayout.get(),
                this->m_uniformBufs.buffers(), tex.view.get(), this->m_sampler1.get()
            );
        }

        this->m_cmdBuffers.init(this->m_logiDevice.get(), this->m_fbuf.getList(), this->m_cmdPool.pool());
        this->m_syncMas.init(this->m_logiDevice.get(), this->m_swapchainImages.size());

        {
            static const std::vector<Vertex> VERTICES = {
                {{-5.f, 0.f, -5.f}, {1.0f, 1.0f, 1.0f}, { 0,  0}},
                {{-5.f, 0.f,  5.f}, {0.0f, 0.0f, 0.0f}, { 0, 10}},
                {{ 5.f, 0.f,  5.f}, {0.0f, 0.0f, 0.0f}, {10, 10}},
                {{ 5.f, 0.f, -5.f}, {0.0f, 0.0f, 0.0f}, {10,  0}},
            };
            static const std::vector<uint16_t> INDICES = {
                0, 1, 2, 0, 2, 3
            };

            this->m_meshes.emplace_back();
            this->m_meshes.back().vertices.init(
                VERTICES, this->m_logiDevice.get(), this->m_physDevice.get(),
                this->m_cmdPool, this->m_logiDevice.graphicsQ()
            );
            this->m_meshes.back().indices.init(
                INDICES, this->m_logiDevice.get(), this->m_physDevice.get(),
                this->m_cmdPool, this->m_logiDevice.graphicsQ()
            );
        }

        {
            static const std::vector<Vertex> VERTICES = {
                {{-0.5f, 0.f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1, 1}}, // 0
                {{-0.5f, 0.f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0, 1}}, // 1
                {{ 0.5f, 0.f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1, 1}}, // 2
                {{ 0.5f, 0.f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0, 1}}, // 3
                {{-0.5f, 1.f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1, 0}}, // 4
                {{-0.5f, 1.f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0, 0}}, // 5
                {{ 0.5f, 1.f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1, 0}}, // 6
                {{ 0.5f, 1.f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0, 0}}, // 7
            };
            static const std::vector<uint16_t> INDICES = {
                0, 2, 1, 0, 3, 2,
                5, 1, 2, 5, 2, 6,
                6, 2, 3, 6, 3, 7,
                7, 3, 0, 7, 0, 4,
                4, 0, 1, 4, 1, 5,
                4, 5, 6, 4, 6, 7
            };

            this->m_meshes.emplace_back();
            this->m_meshes.back().vertices.init(
                VERTICES, this->m_logiDevice.get(), this->m_physDevice.get(),
                this->m_cmdPool, this->m_logiDevice.graphicsQ()
            );
            this->m_meshes.back().indices.init(
                INDICES, this->m_logiDevice.get(), this->m_physDevice.get(),
                this->m_cmdPool, this->m_logiDevice.graphicsQ()
            );
        }

        {
            constexpr float x_offset = 2;
            static const std::vector<Vertex> VERTICES = {
                {{-0.5f + x_offset, 0.f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1, 1}}, // 0
                {{-0.5f + x_offset, 0.f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0, 1}}, // 1
                {{ 0.5f + x_offset, 0.f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1, 1}}, // 2
                {{ 0.5f + x_offset, 0.f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0, 1}}, // 3
                {{-0.5f + x_offset, 1.f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1, 0}}, // 4
                {{-0.5f + x_offset, 1.f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0, 0}}, // 5
                {{ 0.5f + x_offset, 1.f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1, 0}}, // 6
                {{ 0.5f + x_offset, 1.f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0, 0}}, // 7
            };
            static const std::vector<uint16_t> INDICES = {
                0, 2, 1, 0, 3, 2,
                5, 1, 2, 5, 2, 6,
                6, 2, 3, 6, 3, 7,
                7, 3, 0, 7, 0, 4,
                4, 0, 1, 4, 1, 5,
                4, 5, 6, 4, 6, 7
            };

            this->m_meshes.emplace_back();
            this->m_meshes.back().vertices.init(
                VERTICES, this->m_logiDevice.get(), this->m_physDevice.get(),
                this->m_cmdPool, this->m_logiDevice.graphicsQ()
            );
            this->m_meshes.back().indices.init(
                INDICES, this->m_logiDevice.get(), this->m_physDevice.get(),
                this->m_cmdPool, this->m_logiDevice.graphicsQ()
            );
        }

        this->m_cmdBuffers.record(
            this->m_renderPass.get(), this->m_pipeline.getPipeline(), this->m_swapchain.extent(),
            this->m_fbuf.getList(), this->m_pipeline.layout(), this->m_descPool.descSetsList(), this->m_meshes
        );

        this->m_currentFrame = 0;
        this->m_scrWidth = w;
        this->m_scrHeight = h;
    }

    void VulkanMaster::destroy(void) {
        for (auto& mesh : this->m_meshes) {
            mesh.vertices.destroy(this->m_logiDevice.get());
            mesh.indices.destroy(this->m_logiDevice.get());
        }
        this->m_meshes.clear();

        this->m_syncMas.destroy(this->m_logiDevice.get());
        //this->m_cmdBuffers.destroy(this->m_logiDevice.get(), this->m_cmdPool.pool());
        this->m_descPool.destroy(this->m_logiDevice.get());
        this->m_uniformBufs.destroy(this->m_logiDevice.get());

        for (auto& tex : this->m_textures) {
            tex.view.destroy(this->m_logiDevice.get());
            tex.image.destroy(this->m_logiDevice.get());
        }
        this->m_textures.clear();
        this->m_sampler1.destroy(this->m_logiDevice.get());

        this->m_cmdPool.destroy(this->m_logiDevice.get());
        this->m_fbuf.destroy(this->m_logiDevice.get());
        this->m_pipeline.destroy(this->m_logiDevice.get());
        this->m_descSetLayout.destroy(this->m_logiDevice.get());
        this->m_renderPass.destroy(this->m_logiDevice.get());
        this->m_depth_image.destroy(this->m_logiDevice.get());
        this->m_swapchainImages.destroy(this->m_logiDevice.get());
        this->m_swapchain.destroy(this->m_logiDevice.get());
        this->m_logiDevice.destroy();
    }

    void VulkanMaster::render(const VkSurfaceKHR surface) {
        auto& currentFence = this->m_syncMas.fenceInFlight(this->m_currentFrame);
        currentFence.wait(this->m_logiDevice.get());

        const auto imageIndex = this->m_syncMas.acquireGetNextImgIndex(this->m_currentFrame, this->m_logiDevice.get(), this->m_swapchain.get());
        if (this->m_needResize || ::isResizeNeeded(imageIndex.second)) {
            this->recreateSwapChain(surface);
            this->m_needResize = false;
            return;
        }
        else {
            auto& imagesInFlight = this->m_syncMas.fencesImageInFlight();
            // Check if a previous frame is using this image (i.e. there is its fence to wait on)
            if ( imagesInFlight[imageIndex.first] != VK_NULL_HANDLE ) {
                vkWaitForFences(this->m_logiDevice.get(), 1, &imagesInFlight[imageIndex.first], VK_TRUE, UINT64_MAX);
            }
            // Mark the image as now being in use by this frame
            imagesInFlight[imageIndex.first] = this->m_syncMas.fenceInFlight(this->m_currentFrame).get();
        }

        this->m_uniformBufs.update(imageIndex.first, this->m_swapchain.extent(), this->m_logiDevice.get());

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        const std::array<VkSemaphore, 1> waitSemaphores = { this->m_syncMas.semaphImageAvailable(this->m_currentFrame).get() };
        const std::array<VkPipelineStageFlags, 1> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = waitSemaphores.size();
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &this->m_cmdBuffers.buffers()[imageIndex.first];

        const std::array<VkSemaphore, 1> signalSemaphores = { this->m_syncMas.semaphRenderFinished(this->m_currentFrame).get() };
        submitInfo.signalSemaphoreCount = signalSemaphores.size();
        submitInfo.pSignalSemaphores = signalSemaphores.data();

        currentFence.reset(this->m_logiDevice.get());
        const auto submitRes = vkQueueSubmit(this->m_logiDevice.graphicsQ(), 1, &submitInfo, currentFence.get());
        if ( submitRes != VK_SUCCESS ) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = signalSemaphores.size();
        presentInfo.pWaitSemaphores = signalSemaphores.data();
        std::array<VkSwapchainKHR, 1> swapChains = { this->m_swapchain.get() };
        presentInfo.swapchainCount = swapChains.size();
        presentInfo.pSwapchains = swapChains.data();
        presentInfo.pImageIndices = &imageIndex.first;
        presentInfo.pResults = nullptr; // Optional

        vkQueuePresentKHR(this->m_logiDevice.presentQ(), &presentInfo);

        // idk whether do I need to enable this or not.
        // A comment said It's only needed when validation layer is enabled.
        // Check out the comment section in https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Rendering_and_presentation
        //vkQueueWaitIdle(this->m_logiDevice.presentQ());

        this->m_currentFrame = (this->m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void VulkanMaster::waitLogiDeviceIdle(void) const {
        vkDeviceWaitIdle(this->m_logiDevice.get());
    }

    void VulkanMaster::recreateSwapChain(const VkSurfaceKHR surface) {
        this->waitLogiDeviceIdle();

        {
            this->m_syncMas.destroy(this->m_logiDevice.get());
            this->m_cmdBuffers.destroy(this->m_logiDevice.get(), this->m_cmdPool.pool());
            this->m_descPool.destroy(this->m_logiDevice.get());
            this->m_uniformBufs.destroy(this->m_logiDevice.get());
            this->m_fbuf.destroy(this->m_logiDevice.get());
            this->m_pipeline.destroy(this->m_logiDevice.get());
            this->m_renderPass.destroy(this->m_logiDevice.get());
            this->m_depth_image.destroy(this->m_logiDevice.get());
            this->m_swapchainImages.destroy(this->m_logiDevice.get());
            this->m_swapchain.destroy(this->m_logiDevice.get());
        }

        {
            this->m_swapchain.init(surface, this->m_physDevice.get(), this->m_logiDevice.get(), this->m_scrWidth, this->m_scrHeight);
            this->m_swapchainImages.init(this->m_logiDevice.get(), this->m_swapchain.get(), this->m_swapchain.imageFormat(), this->m_swapchain.extent());
            this->m_depth_image.init(this->m_swapchain.extent(), this->m_logiDevice.get(), this->m_physDevice.get());
            this->m_renderPass.init(this->m_logiDevice.get(), this->m_swapchain.imageFormat(), this->m_depth_image.format());
            this->m_pipeline.init(this->m_logiDevice.get(), this->m_renderPass.get(), this->m_swapchain.extent(), this->m_descSetLayout.get());
            this->m_fbuf.init(this->m_logiDevice.get(), this->m_renderPass.get(), this->m_swapchainImages.getViews(), this->m_swapchain.extent(), this->m_depth_image.image_view());
            this->m_uniformBufs.init(this->m_logiDevice.get(), this->m_physDevice.get(), this->m_swapchainImages.size());
            this->m_descPool.initPool(this->m_logiDevice.get(), this->m_swapchainImages.size());
            for (const auto& tex : this->m_textures) {
                this->m_descPool.addSets(
                    this->m_logiDevice.get(), this->m_swapchainImages.size(), this->m_descSetLayout.get(),
                    this->m_uniformBufs.buffers(), tex.view.get(), this->m_sampler1.get()
                );
            }

            this->m_cmdBuffers.init(this->m_logiDevice.get(), this->m_fbuf.getList(), this->m_cmdPool.pool());
            this->m_syncMas.init(this->m_logiDevice.get(), this->m_swapchainImages.size());
        }

        this->m_cmdBuffers.record(
            this->m_renderPass.get(), this->m_pipeline.getPipeline(), this->m_swapchain.extent(),
            this->m_fbuf.getList(), this->m_pipeline.layout(), this->m_descPool.descSetsList(), this->m_meshes
        );
    }

    void VulkanMaster::notifyScreenResize(const unsigned w, const unsigned h) {
        this->m_needResize = true;
        this->m_scrWidth = w;
        this->m_scrHeight = h;
    }

}
