#include "renderpass.h"

#include <array>
#include <stdexcept>


namespace {

    VkRenderPass createRenderpass(const VkDevice logiDevice, const std::array<VkFormat, 5>& attachment_formats) {
        std::array<VkAttachmentDescription, 5> attachments{};
        {
            // Presented
            attachments[0].format = attachment_formats[0];
            attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            // Depth
            attachments[1].format = attachment_formats[1];
            attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            // Position
            attachments[2].format = attachment_formats[2];
            attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // Normal
            attachments[3].format = attachment_formats[3];
            attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[3].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // Albedo
            attachments[4].format = attachment_formats[4];
            attachments[4].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[4].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[4].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[4].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        // Index of color attachment can be used with index number if fragment shader
        // like "layout(location = 0) out vec4 outColor".
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        // Check out https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Rendering_and_presentation
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        VkRenderPass renderPass = VK_NULL_HANDLE;
        if ( VK_SUCCESS != vkCreateRenderPass(logiDevice, &renderPassInfo, nullptr, &renderPass) ) {
            throw std::runtime_error("failed to create render pass!");
        }

        return renderPass;
    }

}


namespace dal {

    void RenderPass::init(const VkDevice logiDevice, const std::array<VkFormat, 5>& attachment_formats) {
        this->m_renderPass = createRenderpass(logiDevice, attachment_formats);
    }

    void RenderPass::destroy(VkDevice logiDevice) {
        if (VK_NULL_HANDLE != this->m_renderPass) {
            vkDestroyRenderPass(logiDevice, this->m_renderPass, nullptr);
            this->m_renderPass = VK_NULL_HANDLE;
        }
    }

}
