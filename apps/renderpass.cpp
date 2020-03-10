#include "renderpass.h"

#include <stdexcept>


namespace {

    VkRenderPass createRenderpass(VkDevice logiDevice, const VkFormat swapChainImageFormat) {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        // Index of color attachment can be used with index number if fragment shader
        // like "layout(location = 0) out vec4 outColor".

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        VkRenderPass renderPass = VK_NULL_HANDLE;
        if ( VK_SUCCESS != vkCreateRenderPass(logiDevice, &renderPassInfo, nullptr, &renderPass) ) {
            throw std::runtime_error("failed to create render pass!");
        }

        return renderPass;
    }

}


namespace dal {

    void RenderPass::init(VkDevice logiDevice, const VkFormat swapChainImageFormat) {
        this->m_renderPass = createRenderpass(logiDevice, swapChainImageFormat);
    }

    void RenderPass::destroy(VkDevice logiDevice) {
        vkDestroyRenderPass(logiDevice, this->m_renderPass, nullptr);
        this->m_renderPass = VK_NULL_HANDLE;
    }

}
