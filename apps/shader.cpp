#include "shader.h"

#include <array>
#include <vector>
#include <fstream>

#define DAL_ALPHA_BLEND false


using namespace std::string_literals;


namespace {

    std::vector<char> readFile(const char* const path) {
        std::ifstream file{ path, std::ios::ate | std::ios::binary };

        if ( !file.is_open() ) {
            throw std::runtime_error("failed to open file: "s + path);
        }

        const auto fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer;
        buffer.resize(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

}


// Vulkan functions
namespace {

    VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device) {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if ( vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS ) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    VkPipelineLayout createGraphicsPipeline(VkDevice device, const VkExtent2D& extent) {
        VkPipelineLayout result;

        const auto vertShaderCode = readFile("shader/triangle_v.spv");
        const auto fragShaderCode = readFile("shader/triangle_f.spv");

        const VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, device);
        const VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, device);

        // Shaders

        VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

        // Vertex input

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;  // Optional
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;  // Optional

        // Input assembly

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewports and scissors

        VkViewport viewport = {};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        // Rasterizer

        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;  // vulkan-tutorial.com said this requires GPU feature enabled.
        rasterizer.rasterizerDiscardEnable = VK_FALSE;  // Discards all fragents. But why would you want it?
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;  // Any mode other than FILL requires CPU feature enabled.
        rasterizer.lineWidth = 1.f;  // GPU feature, wideLines required for lines thicker than 1.
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;  // I prefer ccw but I'mma just follow the tutorial.
        rasterizer.depthBiasEnable = VK_FALSE;  // Maybe this is used to deal with shadow acne?
        rasterizer.depthBiasConstantFactor = 0.f;  // Optional
        rasterizer.depthBiasClamp = 0.f;  // Optional
        rasterizer.depthBiasSlopeFactor = 0.f;  // Optiona

        // Multisampling
        // Which is for anti-aliasing.
        // It requires GPU feature.

        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.f;  // Optional
        multisampling.pSampleMask = nullptr;  // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
        multisampling.alphaToOneEnable = VK_FALSE;  // Optional

        // Depth and stencil testing
        // Don't use atm.

        // Color blending

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};

#if DAL_ALPHA_BLEND
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
#else
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
#endif

        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
#if DAL_ALPHA_BLEND
        colorBlending.logicOpEnable = VK_TRUE;
#else
        colorBlending.logicOpEnable = VK_FALSE;
#endif
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        // Dynamic state
        // Some things can be changed without creating whole pipeline again.
        // Such as the size of the viewport, line width and blend constants.
        // This will cause the configuration of these values to be ignored and you will be required to specify the data at drawing time.

        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_LINE_WIDTH
        };

        VkPipelineDynamicStateCreateInfo dynamicState = {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        // Pipeline layout
        // Which is uniform variable.

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if ( vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &result) != VK_SUCCESS ) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        // Cleap up

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);

        return result;
    }

}


// ShaderPipeline
namespace dal {

    void ShaderPipeline::init(VkDevice device, const VkExtent2D& extent) {
        this->m_pipelineLayout = createGraphicsPipeline(device, extent);
    }

    void ShaderPipeline::destroy(VkDevice device) {
        vkDestroyPipelineLayout(device, this->m_pipelineLayout, nullptr);
    }

}
