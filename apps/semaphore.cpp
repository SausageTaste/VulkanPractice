#include "semaphore.h"

#include <stdexcept>


namespace {

    VkSemaphoreCreateInfo createSemaphoreInfo(void) {
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        return semaphoreInfo;
    }

    VkSemaphore createSemaphore(VkDevice device) {
        static const auto info = createSemaphoreInfo();

        VkSemaphore semaphore = VK_NULL_HANDLE;
        if ( VK_SUCCESS != vkCreateSemaphore(device, &info, nullptr, &semaphore) ) {
            throw std::runtime_error{ "failed to create a semaphore!" };
        }

        return semaphore;
    }

    VkFenceCreateInfo createFenceInfo(void) {
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        return fenceInfo;
    }

    VkFence createFence(VkDevice device) {
        static const auto info = createFenceInfo();

        VkFence fence = VK_NULL_HANDLE;
        if ( VK_SUCCESS != vkCreateFence(device, &info, nullptr, &fence) ) {
            throw std::runtime_error("failed to create a fence!");
        }

        return fence;
    }

}


// Semaphore
namespace dal {

    void Semaphore::init(VkDevice device) {
        this->m_handle = createSemaphore(device);
    }

    void Semaphore::destroy(VkDevice device) {
        vkDestroySemaphore(device, this->m_handle, nullptr);
        this->m_handle = VK_NULL_HANDLE;
    }

}


// Fence
namespace dal {

    void Fence::init(VkDevice device) {
        this->m_handle = createFence(device);
    }

    void Fence::destroy(VkDevice device) {
        vkDestroyFence(device, this->m_handle, nullptr);
        this->m_handle = VK_NULL_HANDLE;
    }

    void Fence::wait(VkDevice device) const {
        vkWaitForFences(device, 1, &this->m_handle, VK_TRUE, UINT64_MAX);
    }

    void Fence::reset(VkDevice device) const {
        vkResetFences(device, 1, &this->m_handle);
    }

}


// SyncMaster
namespace dal {

    void SyncMaster::init(VkDevice device, const unsigned swapChainImgCount) {
        for ( unsigned i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i ) {
            this->m_imageAvailable[i].init(device);
            this->m_renderFinished[i].init(device);
            this->m_inFlightFences[i].init(device);
        }

        this->m_imageInFlight.resize(swapChainImgCount, VK_NULL_HANDLE);
    }

    void SyncMaster::destroy(VkDevice device) {
        this->m_imageInFlight.clear();

        for ( unsigned i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i ) {
            this->m_inFlightFences[i].destroy(device);
            this->m_renderFinished[i].destroy(device);
            this->m_imageAvailable[i].destroy(device);
        }
    }

    std::pair<uint32_t, VkResult> SyncMaster::acquireGetNextImgIndex(const unsigned index, VkDevice device, VkSwapchainKHR swapChain) {
        uint32_t imageIndex;
        const VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, this->m_imageAvailable[index].get(), VK_NULL_HANDLE, &imageIndex);
        return {imageIndex, result};
    }

}
