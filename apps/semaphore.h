#pragma once

#include <array>
#include <vector>

#include <vulkan/vulkan.h>

#include "konst.h"


namespace dal {

    class Semaphore {

    private:
        VkSemaphore m_handle = VK_NULL_HANDLE;

    public:
        void init(VkDevice device);
        void destroy(VkDevice device);

        auto get(void) const {
            return this->m_handle;
        }

    };


    class Fence {

    private:
        VkFence m_handle = VK_NULL_HANDLE;

    public:
        void init(VkDevice device);
        void destroy(VkDevice device);

        auto& get(void) {
            return this->m_handle;
        }
        auto& get(void) const {
            return this->m_handle;
        }
        bool isReady(void) const {
            return VK_NULL_HANDLE != this->m_handle;
        }

        void wait(VkDevice device) const;
        void reset(VkDevice device) const;

    };


    class SyncMaster {

    private:
        std::array<Semaphore, MAX_FRAMES_IN_FLIGHT> m_imageAvailable, m_renderFinished;
        std::array<Fence, MAX_FRAMES_IN_FLIGHT> m_inFlightFences;
        std::vector<VkFence> m_imageInFlight;

    public:
        void init(VkDevice device, const unsigned swapChainImgCount);
        void destroy(VkDevice device);

        auto& semaphImageAvailable(const unsigned index) const {
            return this->m_imageAvailable[index];
        }
        auto& semaphRenderFinished(const unsigned index) const {
            return this->m_renderFinished[index];
        }
        auto& fenceInFlight(const unsigned index) const {
            return this->m_inFlightFences[index];
        }

        auto& fencesImageInFlight(void) {
            return this->m_imageInFlight;
        }
        auto& fencesImageInFlight(void) const {
            return this->m_imageInFlight;
        }

        uint32_t acquireGetNextImgIndex(const unsigned index, VkDevice device, VkSwapchainKHR swapChain);

    };

}
