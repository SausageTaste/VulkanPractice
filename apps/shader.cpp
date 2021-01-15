#include "shader.h"

#include <tuple>
#include <array>
#include <vector>
#include <fstream>

#include "util_windows.h"
#include "vert_data.h"
#include "model_data.h"


#define DAL_ALPHA_BLEND false


// Shader module tools
namespace {

    class ShaderModule {

    private:
        const VkDevice m_parent_device;
        VkShaderModule m_module = VK_NULL_HANDLE;

    public:
        ShaderModule(const VkDevice logi_device)
            : m_parent_device(logi_device)
        {

        }

        ShaderModule(const VkDevice logi_device, const char* const source_str, const uint32_t source_size)
            : m_parent_device(logi_device)
        {
            this->init(source_str, source_size);
        }

        ~ShaderModule() {
            this->destroy();
        }

        void init(const char* const source_str, const uint32_t source_size) {
            VkShaderModuleCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = source_size;
            createInfo.pCode = reinterpret_cast<const uint32_t*>(source_str);

            if (VK_SUCCESS != vkCreateShaderModule(this->m_parent_device, &createInfo, nullptr, &this->m_module)) {
                throw std::runtime_error("failed to create shader module!");
            }
        }

        void destroy() {
            if (VK_NULL_HANDLE != this->m_module) {
                vkDestroyShaderModule(this->m_parent_device, this->m_module, nullptr);
                this->m_module = VK_NULL_HANDLE;
            }
        }

        auto& get() const {
            return this->m_module;
        }

    };

}

// Pipeline creation functions
namespace {

    auto create_info_shader_stage(const ShaderModule& vert_shader_module, const ShaderModule& frag_shader_module) {
        std::array<VkPipelineShaderStageCreateInfo, 2> result{};

        result[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        result[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        result[0].module = vert_shader_module.get();
        result[0].pName = "main";

        result[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        result[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        result[1].module = frag_shader_module.get();
        result[1].pName = "main";

        return result;
    }

    auto create_vertex_input_state(
        const   VkVertexInputBindingDescription* const binding_descriptions, const uint32_t binding_count,
        const VkVertexInputAttributeDescription* const attrib_descriptions,  const uint32_t attrib_count
    ) {
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        vertexInputInfo.vertexBindingDescriptionCount = binding_count;
        vertexInputInfo.pVertexBindingDescriptions = binding_descriptions;
        vertexInputInfo.vertexAttributeDescriptionCount = attrib_count;
        vertexInputInfo.pVertexAttributeDescriptions = attrib_descriptions;

        return vertexInputInfo;
    }

    auto create_info_input_assembly() {
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        return inputAssembly;
    }

    auto create_info_viewport_scissor(const VkExtent2D& extent) {
        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0;
        viewport.maxDepth = 1;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;

        return std::make_pair(viewport, scissor);
    }

    auto create_info_viewport_state(
        const VkViewport* const viewports, const uint32_t viewport_count,
        const   VkRect2D* const scissors,  const uint32_t scissor_count
    ) {
        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

        viewportState.viewportCount = viewport_count;
        viewportState.pViewports = viewports;
        viewportState.scissorCount = scissor_count;
        viewportState.pScissors = scissors;

        return viewportState;
    }

    auto create_info_rasterizer(
        const VkCullModeFlags cull_mode,
        const bool enable_bias = VK_FALSE,
        const float bias_constant = 0,
        const float bias_slope = 0
    ) {
        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

        rasterizer.depthClampEnable = VK_FALSE;  // vulkan-tutorial.com said this requires GPU feature enabled.
        rasterizer.rasterizerDiscardEnable = VK_FALSE;  // Discards all fragents. But why would you want it?
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;  // Any mode other than FILL requires CPU feature enabled.
        rasterizer.lineWidth = 1;  // GPU feature, wideLines required for lines thicker than 1.
        rasterizer.cullMode = cull_mode;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = enable_bias;  // Maybe this is used to deal with shadow acne?
        rasterizer.depthBiasConstantFactor = bias_constant;
        rasterizer.depthBiasSlopeFactor = bias_slope;
        rasterizer.depthBiasClamp = 0;  // Optional

        return rasterizer;
    }

    auto create_info_multisampling() {
        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1;  // Optional
        multisampling.pSampleMask = nullptr;  // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
        multisampling.alphaToOneEnable = VK_FALSE;  // Optional

        return multisampling;
    }

    template <size_t _Size, bool _AlphaBlend>
    auto create_info_color_blend_attachment() {
        VkPipelineColorBlendAttachmentState attachment_state_template{};

        attachment_state_template.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        attachment_state_template.colorBlendOp = VK_BLEND_OP_ADD;
        attachment_state_template.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        attachment_state_template.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        attachment_state_template.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        if constexpr (_AlphaBlend) {
            attachment_state_template.blendEnable = VK_TRUE;
            attachment_state_template.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            attachment_state_template.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        }
        else {
            attachment_state_template.blendEnable = VK_FALSE;
            attachment_state_template.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
            attachment_state_template.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        }

        std::array<VkPipelineColorBlendAttachmentState, _Size> result;

        for (auto& x : result) {
            x = attachment_state_template;
        }

        return result;
    }

    auto create_info_color_blend(
        const VkPipelineColorBlendAttachmentState* const color_blend_attachments,
        const uint32_t attachment_count, const bool alpha_blend
    ) {
        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

        colorBlending.logicOpEnable = alpha_blend ? VK_TRUE : VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = attachment_count;
        colorBlending.pAttachments = color_blend_attachments;
        colorBlending.blendConstants[0] = 0; // Optional
        colorBlending.blendConstants[1] = 0; // Optional
        colorBlending.blendConstants[2] = 0; // Optional
        colorBlending.blendConstants[3] = 0; // Optional

        return colorBlending;
    }

    auto create_info_depth_stencil(const bool depth_write) {
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = depth_write ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0; // Optional
        depthStencil.maxDepthBounds = 1; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional

        return depthStencil;
    }

    auto create_info_dynamic_state(const VkDynamicState* const dynamic_states, const uint32_t dynamic_count) {
        VkPipelineDynamicStateCreateInfo dynamicState = {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

        dynamicState.dynamicStateCount = dynamic_count;
        dynamicState.pDynamicStates = dynamic_states;

        return dynamicState;
    }

    template <typename _Struct>
    auto create_info_push_constant() {
        std::array<VkPushConstantRange, 1> result;

        result[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        result[0].offset = 0;
        result[0].size = sizeof(_Struct);

        return result;
    }

    auto create_pipeline_layout(
        const VkDescriptorSetLayout* const layouts, const uint32_t layout_count,
        const VkPushConstantRange* const push_consts, const uint32_t push_const_count,
        const VkDevice logi_device
    ) {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        pipelineLayoutInfo.setLayoutCount = layout_count;
        pipelineLayoutInfo.pSetLayouts = layouts;
        pipelineLayoutInfo.pushConstantRangeCount = push_const_count;
        pipelineLayoutInfo.pPushConstantRanges = push_consts;

        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        if ( vkCreatePipelineLayout(logi_device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS ) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        return pipelineLayout;
    }


    auto createGraphicsPipeline_deferred(const VkDevice device, VkRenderPass renderPass, const VkExtent2D& extent, const VkDescriptorSetLayout descriptorSetLayout) {
        // Shaders
        const auto vertShaderCode = dal::readFile(dal::get_res_path() + "/shader/triangle_v.spv");
        const auto fragShaderCode = dal::readFile(dal::get_res_path() + "/shader/triangle_f.spv");
        const ShaderModule vert_shader_module(device, vertShaderCode.data(), vertShaderCode.size());
        const ShaderModule frag_shader_module(device, fragShaderCode.data(), fragShaderCode.size());
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = ::create_info_shader_stage(vert_shader_module, frag_shader_module);

        // Vertex input
        const auto bindingDesc = dal::getBindingDesc();
        const auto attribDesc = dal::getAttributeDescriptions();
        auto vertexInputInfo = ::create_vertex_input_state(&bindingDesc, 1, attribDesc.data(), attribDesc.size());

        // Input assembly
        const VkPipelineInputAssemblyStateCreateInfo inputAssembly = ::create_info_input_assembly();

        // Viewports and scissors
        const auto [viewport, scissor] = ::create_info_viewport_scissor(extent);
        const auto viewportState = ::create_info_viewport_state(&viewport, 1, &scissor, 1);

        // Rasterizer
        const auto rasterizer = ::create_info_rasterizer(VK_CULL_MODE_BACK_BIT);

        // Multisampling
        const auto multisampling = ::create_info_multisampling();

        // Color blending
        const auto colorBlendAttachments = ::create_info_color_blend_attachment<4, false>();
        const auto colorBlending = ::create_info_color_blend(colorBlendAttachments.data(), colorBlendAttachments.size(), false);

        // Depth, stencil
        const auto depthStencil = ::create_info_depth_stencil(true);

        // Dynamic state
        constexpr std::array<VkDynamicState, 0> dynamicStates{};
        const auto dynamicState = ::create_info_dynamic_state(dynamicStates.data(), dynamicStates.size());

        // Pipeline layout
        const auto push_consts = ::create_info_push_constant<dal::PushedConstValues>();
        const auto pipelineLayout = ::create_pipeline_layout(&descriptorSetLayout, 1, push_consts.data(), push_consts.size(), device);

        // Pipeline, finally
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = shaderStages.size();
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        VkPipeline graphicsPipeline = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline)) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        return std::make_pair(pipelineLayout, graphicsPipeline);
    }

    auto createGraphicsPipeline_composition(const VkDevice device, VkRenderPass renderPass, const VkExtent2D& extent, const VkDescriptorSetLayout descriptorSetLayout) {
        // Shaders
        const auto vertShaderCode = dal::readFile(dal::get_res_path() + "/shader/fillsc_v.spv");
        const auto fragShaderCode = dal::readFile(dal::get_res_path() + "/shader/fillsc_f.spv");
        const ShaderModule vert_shader_module(device, vertShaderCode.data(), vertShaderCode.size());
        const ShaderModule frag_shader_module(device, fragShaderCode.data(), fragShaderCode.size());
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = ::create_info_shader_stage(vert_shader_module, frag_shader_module);

        // Vertex input
        const auto vertexInputInfo = ::create_vertex_input_state(nullptr, 0, nullptr, 0);

        // Input assembly
        const auto inputAssembly = ::create_info_input_assembly();

        // Viewports and scissors
        const auto [viewport, scissor] = ::create_info_viewport_scissor(extent);
        const auto viewportState = ::create_info_viewport_state(&viewport, 1, &scissor, 1);

        // Rasterizer
        auto rasterizer = ::create_info_rasterizer(VK_CULL_MODE_NONE);

        // Multisampling
        const auto multisampling = ::create_info_multisampling();

        // Color blending
        const auto colorBlendAttachments = ::create_info_color_blend_attachment<1, false>();
        const auto colorBlending = ::create_info_color_blend(
            colorBlendAttachments.data(), colorBlendAttachments.size(), false
        );

        // Depth, stencil
        const auto depthStencil = ::create_info_depth_stencil(false);

        // Dynamic state
        constexpr std::array<VkDynamicState, 0> dynamicStates{};
        const auto dynamicState = ::create_info_dynamic_state(dynamicStates.data(), dynamicStates.size());

        // Pipeline layout
        VkPipelineLayout pipelineLayout = ::create_pipeline_layout(&descriptorSetLayout, 1, nullptr, 0, device);

        // Pipeline, finally
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = shaderStages.size();
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 1;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        VkPipeline graphicsPipeline = VK_NULL_HANDLE;
        if ( vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS ) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        return std::make_pair(pipelineLayout, graphicsPipeline);
    }

    auto createGraphicsPipeline_shadow(const VkDevice device, VkRenderPass renderPass, const VkExtent2D& extent, const VkDescriptorSetLayout descriptorSetLayout) {
        // Shaders
        const auto vertShaderCode = dal::readFile(dal::get_res_path() + "/shader/shadow_map_v.spv");
        const auto fragShaderCode = dal::readFile(dal::get_res_path() + "/shader/shadow_map_f.spv");
        const ShaderModule vert_shader_module(device, vertShaderCode.data(), vertShaderCode.size());
        const ShaderModule frag_shader_module(device, fragShaderCode.data(), fragShaderCode.size());
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = ::create_info_shader_stage(vert_shader_module, frag_shader_module);

        // Vertex input
        const auto bindingDesc = dal::getBindingDesc();
        const auto attribDesc = dal::getAttributeDescriptions();
        auto vertexInputInfo = ::create_vertex_input_state(&bindingDesc, 1, attribDesc.data(), attribDesc.size());

        // Input assembly
        const auto inputAssembly = ::create_info_input_assembly();

        // Viewports and scissors
        const auto [viewport, scissor] = ::create_info_viewport_scissor(extent);
        const auto viewportState = ::create_info_viewport_state(&viewport, 1, &scissor, 1);

        // Rasterizer
        auto rasterizer = ::create_info_rasterizer(VK_CULL_MODE_NONE, true, 4, 1.5);

        // Multisampling
        const auto multisampling = ::create_info_multisampling();

        // Color blending
        const auto colorBlendAttachments = ::create_info_color_blend_attachment<0, false>();
        const auto colorBlending = ::create_info_color_blend(
            colorBlendAttachments.data(), colorBlendAttachments.size(), false
        );

        // Depth, stencil
        const auto depthStencil = ::create_info_depth_stencil(true);

        // Dynamic state
        constexpr std::array<VkDynamicState, 0> dynamicStates{};
        const auto dynamicState = ::create_info_dynamic_state(dynamicStates.data(), dynamicStates.size());

        // Pipeline layout
        const auto push_consts = ::create_info_push_constant<dal::PushedConstValues>();
        const auto pipelineLayout = ::create_pipeline_layout(&descriptorSetLayout, 1, push_consts.data(), push_consts.size(), device);

        // Pipeline, finally
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = shaderStages.size();
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        VkPipeline graphicsPipeline = VK_NULL_HANDLE;
        if ( vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS ) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        return std::make_pair(pipelineLayout, graphicsPipeline);
    }

}


// ShaderPipeline
namespace dal {

    void ShaderPipeline::init(
        const VkDevice device,
        const VkRenderPass renderPass,
        const VkRenderPass shadow_renderpass,
        const VkExtent2D& extent,
        const VkExtent2D& shadow_extent,
        const VkDescriptorSetLayout desc_layout_deferred,
        const VkDescriptorSetLayout desc_layout_composition,
        const VkDescriptorSetLayout desc_layout_shadow
    ) {
        std::tie(this->m_layout_deferred, this->m_pipeline_deferred) = ::createGraphicsPipeline_deferred(device, renderPass, extent, desc_layout_deferred);
        std::tie(this->m_layout_composition, this->m_pipeline_composition) = ::createGraphicsPipeline_composition(device, renderPass, extent, desc_layout_composition);
        std::tie(this->m_layout_shadow, this->m_pipeline_shadow) = ::createGraphicsPipeline_shadow(device, shadow_renderpass, shadow_extent, desc_layout_shadow);
    }

    void ShaderPipeline::destroy(VkDevice device) {
        if (VK_NULL_HANDLE != this->m_layout_deferred) {
            vkDestroyPipelineLayout(device, this->m_layout_deferred, nullptr);
            this->m_layout_deferred = VK_NULL_HANDLE;
        }
        if (VK_NULL_HANDLE != this->m_layout_composition) {
            vkDestroyPipelineLayout(device, this->m_layout_composition, nullptr);
            this->m_layout_composition = VK_NULL_HANDLE;
        }
        if (VK_NULL_HANDLE != this->m_layout_shadow) {
            vkDestroyPipelineLayout(device, this->m_layout_shadow, nullptr);
            this->m_layout_shadow = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != this->m_pipeline_deferred) {
            vkDestroyPipeline(device, this->m_pipeline_deferred, nullptr);
            this->m_pipeline_deferred = VK_NULL_HANDLE;
        }
        if (VK_NULL_HANDLE != this->m_pipeline_composition) {
            vkDestroyPipeline(device, this->m_pipeline_composition, nullptr);
            this->m_pipeline_composition = VK_NULL_HANDLE;
        }
        if (VK_NULL_HANDLE != this->m_pipeline_shadow) {
            vkDestroyPipeline(device, this->m_pipeline_shadow, nullptr);
            this->m_pipeline_shadow = VK_NULL_HANDLE;
        }
    }

}
