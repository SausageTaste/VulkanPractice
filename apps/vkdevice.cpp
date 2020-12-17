#include "vkdevice.h"

#include <array>
#include <stdexcept>


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
        this->m_demoVertBuf.init(dal::getDemoVertices(), this->m_logiDevice.get(), this->m_physDevice.get());

        this->initSwapChain(surface);

        this->m_demoVertBuf.init(dal::getDemoVertices(), this->m_logiDevice.get(), this->m_physDevice.get());

        this->m_currentFrame = 0;
        this->m_scrWidth = w;
        this->m_scrHeight = h;
    }

    void VulkanMaster::destroy(void) {
        this->destroySwapChain();

        this->m_demoVertBuf.destroy(this->m_logiDevice.get());
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

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        const std::array<VkSemaphore, 1> waitSemaphores = { this->m_syncMas.semaphImageAvailable(this->m_currentFrame).get() };
        const std::array <VkPipelineStageFlags, 1> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = waitSemaphores.size();
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &this->m_command.buffers()[imageIndex.first];

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
        this->destroySwapChain();
        this->initSwapChain(surface);
    }

    void VulkanMaster::notifyScreenResize(const unsigned w, const unsigned h) {
        this->m_needResize = true;
        this->m_scrWidth = w;
        this->m_scrHeight = h;
    }

    void VulkanMaster::initSwapChain(const VkSurfaceKHR surface) {
        this->m_swapchain.init(surface, this->m_physDevice.get(), this->m_logiDevice.get(), this->m_scrWidth, this->m_scrHeight);
        this->m_swapchainImages.init(this->m_logiDevice.get(), this->m_swapchain.get(), this->m_swapchain.imageFormat(), this->m_swapchain.extent());
        this->m_renderPass.init(this->m_logiDevice.get(), this->m_swapchain.imageFormat());
        this->m_pipeline.init(this->m_logiDevice.get(), this->m_renderPass.get(), this->m_swapchain.extent());
        this->m_fbuf.init(this->m_logiDevice.get(), this->m_renderPass.get(), this->m_swapchainImages.getViews(), this->m_swapchain.extent());
        this->m_command.initPool(this->m_physDevice.get(), this->m_logiDevice.get(), surface);
        this->m_command.initCmdBuffers(
            this->m_logiDevice.get(), this->m_renderPass.get(), this->m_pipeline.getPipeline(), this->m_swapchain.extent(),
            this->m_fbuf.getList(), this->m_demoVertBuf.getBuf(), this->m_demoVertBuf.size()
        );
        this->m_syncMas.init(this->m_logiDevice.get(), this->m_swapchainImages.size());
    }

    void VulkanMaster::destroySwapChain() {
        this->m_syncMas.destroy(this->m_logiDevice.get());
        this->m_command.destroy(this->m_logiDevice.get());
        this->m_fbuf.destroy(this->m_logiDevice.get());
        this->m_pipeline.destroy(this->m_logiDevice.get());
        this->m_renderPass.destroy(this->m_logiDevice.get());
        this->m_swapchainImages.destroy(this->m_logiDevice.get());
        this->m_swapchain.destroy(this->m_logiDevice.get());
    }

}
