#include "model_render.h"

#include <stdexcept>

#include "util_vulkan.h"


// DescSetTensor_Shadow
namespace dal {

    void DescSetTensor_Shadow::init(const VkDevice logi_device) {
        this->m_pool.init(150, 150, 150, 150, logi_device);
    }

    void DescSetTensor_Shadow::destroy(const VkDevice logi_device) {
        this->m_pool.destroy(logi_device);
        this->m_desc_sets.clear();
    }

    void DescSetTensor_Shadow::reset(
        const std::vector<ModelVK>& models,
        const std::vector<DirectionalLight> dlights,
        const uint32_t swapchain_count,
        const VkDescriptorSetLayout desc_layout_shadow,
        const VkDevice logi_device
    ) {
        const auto max_inst_count = [&models]() {
            uint32_t result = 0;

            for (auto& x : models) {
                if (x.instances().size() > result) {
                    result = x.instances().size();
                }
            }

            return result;
        }();

        this->m_pool.reset(logi_device);
        this->m_desc_sets.reset({
            swapchain_count,
            static_cast<uint32_t>(dlights.size()),
            static_cast<uint32_t>(models.size()),
            max_inst_count,
        });

        for (uint32_t dlight_index = 0; dlight_index < dlights.size(); ++dlight_index) {
            auto& dlight = dlights.at(dlight_index);

            for (uint32_t model_index = 0; model_index < models.size(); ++model_index) {
                auto& model = models.at(model_index);

                for (uint32_t inst_index = 0; inst_index < model.instances().size(); ++inst_index) {
                    for (uint32_t swapchain_index = 0; swapchain_index < swapchain_count; ++swapchain_index) {
                        auto inst = model.instances().at(inst_index);
                        auto& one = this->at(swapchain_index, dlight_index, model_index, inst_index);
                        one = this->m_pool.allocate(desc_layout_shadow, logi_device);

                        one.record_shadow(
                            inst.uniform_buffers().buffer_at(swapchain_index),
                            dlight.m_ubufs.buffer_at(swapchain_index),
                            logi_device
                        );
                    }
                }
            }
        }
    }

    DescSet& DescSetTensor_Shadow::at(const uint32_t swapchain_index, const uint32_t dlight_index, const uint32_t model_index, const uint32_t inst_index) {
        return this->m_desc_sets.at({ swapchain_index, dlight_index, model_index, inst_index });
    }

    const DescSet& DescSetTensor_Shadow::at(const uint32_t swapchain_index, const uint32_t dlight_index, const uint32_t model_index, const uint32_t inst_index) const {
        return this->m_desc_sets.at({ swapchain_index, dlight_index, model_index, inst_index });
    }

}


// MaterialVK
namespace dal {

    void MaterialVK::destroy(const VkDevice logi_device) {
        this->m_material_buffer.destroy(logi_device);
    }

    void MaterialVK::set_material(
        const VkImageView texture_image_view,
        const VkDevice logi_device,
        const VkPhysicalDevice phys_device
    ) {
        this->m_albedo_map = texture_image_view;
        this->m_material_buffer.init(logi_device, phys_device);
        this->m_material_buffer.copy_to_buffer(this->m_material_data, logi_device);
    }

}


// RenderUnitVK
namespace dal {

    void RenderUnitVK::set_mesh(
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        dal::CommandPool& cmd_pool,
        const VkDevice logi_device,
        const VkPhysicalDevice phys_device,
        const VkQueue graphics_queue
    ) {
        this->m_mesh.vertices.init(vertices, logi_device, phys_device, cmd_pool, graphics_queue);
        this->m_mesh.indices.init(indices, logi_device, phys_device, cmd_pool, graphics_queue);
    }

}


// ModelInstance
namespace dal {

    void ModelInstance::init(const uint32_t swapchain_count, const VkDevice logi_device, const VkPhysicalDevice phys_device) {
        this->m_ubuf.init(swapchain_count, logi_device, phys_device);
        this->update_ubuf(logi_device);
    }

    void ModelInstance::destroy(const VkDevice logi_device) {
        this->m_ubuf.destroy(logi_device);
    }

    void ModelInstance::update_ubuf(const VkDevice logi_device) {
        U_PerInst_PerFrame_InDeferred data;
        data.m_model_mat = this->m_transform.make_mat();

        for (int i = 0; i < this->m_ubuf.array_size(); ++i) {
            this->m_ubuf.copy_to_buffer(i, data, logi_device);
        }
    }

    void ModelInstance::update_ubuf(const uint32_t index, const VkDevice logi_device) {
        U_PerInst_PerFrame_InDeferred data;
        data.m_model_mat = this->m_transform.make_mat();
        this->m_ubuf.copy_to_buffer(index, data, logi_device);
    }

}


// ModelVK :: DescSet2D
namespace dal {

    void ModelVK::DescSet2D::init(const VkDevice logi_device) {
        this->m_pool.init(512, 512, 512, 512, logi_device);
    }

    void ModelVK::DescSet2D::destroy(const VkDevice logi_device) {
        this->m_pool.destroy(logi_device);
        this->m_desc_sets.clear();
    }

    DescSet& ModelVK::DescSet2D::at(const uint32_t swapchain_index, const uint32_t inst_index, const uint32_t unit_index) {
        return this->m_desc_sets.at({ inst_index, unit_index, swapchain_index });
    }
    const DescSet& ModelVK::DescSet2D::at(const uint32_t swapchain_index, const uint32_t inst_index, const uint32_t unit_index) const {
        return this->m_desc_sets.at({ inst_index, unit_index, swapchain_index });
    }

    void ModelVK::DescSet2D::reset(
        const std::vector<ModelInstance>& insts,
        const std::vector<RenderUnitVK>& units,
        const UniformBufferArray<U_PerFrame_InDeferred>& ubuf_per_frame_in_deferred,
        const uint32_t swapchain_count,
        const VkSampler texture_sampler,
        const VkDescriptorSetLayout desc_layout_deferred,
        const VkDevice logi_device
    ) {
        this->m_pool.reset(logi_device);
        this->m_desc_sets.reset({ static_cast<uint32_t>(insts.size()), static_cast<uint32_t>(units.size()), swapchain_count });

        for (uint32_t inst_index = 0; inst_index < insts.size(); ++inst_index) {
            for (uint32_t unit_index = 0; unit_index < units.size(); ++unit_index) {
                for (uint32_t swapchain_index = 0; swapchain_index < swapchain_count; ++swapchain_index) {
                    auto& one = this->at(swapchain_index, inst_index, unit_index);
                    one = this->m_pool.allocate(desc_layout_deferred, logi_device);
                    one.record_deferred(
                        ubuf_per_frame_in_deferred.buffer_at(swapchain_index),
                        units.at(unit_index).m_material.m_material_buffer,
                        insts.at(inst_index).uniform_buffers().buffer_at(swapchain_index),
                        units.at(unit_index).m_material.m_albedo_map,
                        texture_sampler,
                        logi_device
                    );
                }
            }
        }
    }

}


// ModelVK
namespace dal {

    void ModelVK::destroy(const VkDevice logi_device) {
        this->m_desc_sets.destroy(logi_device);

        for (auto& unit : this->m_render_units) {
            unit.m_mesh.vertices.destroy(logi_device);
            unit.m_mesh.indices.destroy(logi_device);
            unit.m_material.destroy(logi_device);
        }
        this->m_render_units.clear();

        for (auto& inst : this->m_instances) {
            inst.destroy(logi_device);
        }
        this->m_instances.clear();
    }

    void ModelVK::reset_desc_sets(
        const UniformBufferArray<U_PerFrame_InDeferred>& ubuf_per_frame_in_deferred,
        const uint32_t swapchain_count,
        const VkSampler texture_sampler,
        const VkDescriptorSetLayout desc_layout_deferred,
        const VkDevice logi_device
    ) {
        this->m_desc_sets.reset(
            this->m_instances,
            this->m_render_units,
            ubuf_per_frame_in_deferred,
            swapchain_count,
            texture_sampler,
            desc_layout_deferred,
            logi_device
        );
    }

    RenderUnitVK& ModelVK::add_unit() {
        return this->m_render_units.emplace_back();
    }

    ModelInstance& ModelVK::add_instance(const uint32_t swapchain_count, const VkDevice logi_device, const VkPhysicalDevice phys_device) {
        auto& inst = this->m_instances.emplace_back();
        inst.init(swapchain_count, logi_device, phys_device);
        return inst;
    }

}


// DepthMap
namespace dal {

    void DepthMap::init(
        const VkRenderPass renderpass_shadow,
        const VkDevice logi_device,
        const VkPhysicalDevice phys_device
    ) {
        assert(VK_NULL_HANDLE != renderpass_shadow);
        assert(VK_NULL_HANDLE != logi_device);
        assert(VK_NULL_HANDLE != phys_device);

        this->destroy(logi_device);
        this->m_attachment.init(
            logi_device,
            phys_device,
            VK_FORMAT_D32_SFLOAT,
            FbufAttachment::Usage::depth_map,
            SHADOW_MAP_EXTENT.width,
            SHADOW_MAP_EXTENT.height
        );

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderpass_shadow;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &this->m_attachment.view();
        framebufferInfo.width = this->m_attachment.width();
        framebufferInfo.height = this->m_attachment.height();
        framebufferInfo.layers = 1;

        if (VK_SUCCESS != vkCreateFramebuffer(logi_device, &framebufferInfo, nullptr, &this->m_fbuf)) {
            throw std::runtime_error("failed to create framebuffer for depth map");
        }
    }

    void DepthMap::destroy(const VkDevice logi_device) {
        this->m_attachment.destroy(logi_device);

        if (VK_NULL_HANDLE != this->m_fbuf) {
            vkDestroyFramebuffer(logi_device, this->m_fbuf, nullptr);
            this->m_fbuf = VK_NULL_HANDLE;
        }
    }

}


// DirectionalLight
namespace dal {

    void DirectionalLight::init(
        const uint32_t swapchain_count,
        const VkRenderPass renderpass_shadow,
        const VkCommandPool cmd_pool,
        const VkDevice logi_device,
        const VkPhysicalDevice phys_device
    ) {
        this->destroy(cmd_pool, logi_device);
        this->m_depth_map.init(renderpass_shadow, logi_device, phys_device);

        this->m_ubufs.init(swapchain_count, logi_device, phys_device);
        for (uint32_t i = 0; i < this->m_ubufs.array_size(); ++i) {
            this->update_ubuf_at(i, logi_device);
        }

        // Allocate command buffers
        // ------------------------------------------------------------------------------

        this->m_cmd_bufs.resize(swapchain_count);

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = cmd_pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = swapchain_count;

        dal::assert_vk_success(
            vkAllocateCommandBuffers(logi_device, &allocInfo, this->m_cmd_bufs.data())
        );
    }

    void DirectionalLight::destroy(const VkCommandPool cmd_pool, const VkDevice logi_device) {
        this->m_depth_map.destroy(logi_device);
        this->m_ubufs.destroy(logi_device);

        if (!this->m_cmd_bufs.empty()) {
            vkFreeCommandBuffers(logi_device, cmd_pool, this->m_cmd_bufs.size(), this->m_cmd_bufs.data());
            this->m_cmd_bufs.clear();
        }

        this->m_use_shadow = false;
    }

    void DirectionalLight::update_cmd_buf(
        const uint32_t swapchain_count,
        const uint32_t dlight_index,
        const std::vector<ModelVK>& models,
        const DescSetTensor_Shadow& descsets_shadow,
        const VkRenderPass renderpass_shadow,
        const VkPipeline pipeline_shadow,
        const VkPipelineLayout pipelayout_shadow
    ) {
        // Record command buffers
        // ------------------------------------------------------------------------------

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        std::array<VkClearValue, 1> clear_values{};
        clear_values[0].depthStencil = {1.f, 0};

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderpass_shadow;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = this->m_depth_map.extent();
        renderPassInfo.clearValueCount = clear_values.size();
        renderPassInfo.pClearValues = clear_values.data();

        for (uint32_t i = 0; i < swapchain_count; ++i) {
            auto& cmd_buf = this->m_cmd_bufs.at(i);

            dal::assert_vk_success( vkBeginCommandBuffer(cmd_buf, &beginInfo) );

            renderPassInfo.framebuffer = this->m_depth_map.framebuffer();
            vkCmdBeginRenderPass(cmd_buf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_shadow);

            for (uint32_t model_index = 0; model_index < models.size(); ++model_index) {
                auto& model = models.at(model_index);

                for (const auto& render_unit : model.render_units()) {
                    VkBuffer vertBuffers[] = {render_unit.m_mesh.vertices.getBuf()};
                    VkDeviceSize offsets[] = {0};
                    vkCmdBindVertexBuffers(cmd_buf, 0, 1, vertBuffers, offsets);
                    vkCmdBindIndexBuffer(cmd_buf, render_unit.m_mesh.indices.getBuf(), 0, VK_INDEX_TYPE_UINT32);

                    for (uint32_t inst_index = 0; inst_index < model.instances().size(); ++inst_index) {
                        auto& inst = model.instances().at(inst_index);

                        vkCmdBindDescriptorSets(
                            cmd_buf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelayout_shadow,
                            0, 1, &descsets_shadow.at(i, dlight_index, model_index, inst_index).get(), 0, nullptr
                        );

                        vkCmdDrawIndexed(cmd_buf, render_unit.m_mesh.indices.size(), 1, 0, 0, 0);
                    }
                }
            }

            vkCmdEndRenderPass(cmd_buf);

            dal::assert_vk_success( vkEndCommandBuffer(cmd_buf) );
        }

        this->m_use_shadow = true;
    }

    void DirectionalLight::update_ubuf_at(const size_t index, const VkDevice logi_device) {
        U_PerFrame_PerLight data;
        data.m_light_mat = this->make_light_mat();
        this->m_ubufs.copy_to_buffer(index, data, logi_device);
    }

    glm::mat4 DirectionalLight::make_light_mat() const {
        constexpr float half_proj_box_edge_length = 4;

        const auto view_mat = glm::lookAt(-this->m_direc + this->m_pos, this->m_pos, glm::vec3{0, 1, 0});

        auto proj_mat = glm::ortho<float>(
            -half_proj_box_edge_length, half_proj_box_edge_length,
            -half_proj_box_edge_length, half_proj_box_edge_length,
            -half_proj_box_edge_length, half_proj_box_edge_length
        );
        proj_mat[1][1] *= -1;

        return proj_mat * view_mat;
    }

}


// LightManager
namespace dal {

    void LightManager::destroy(const VkCommandPool cmd_pool, const VkDevice logi_device) {
        for (auto& dlight : this->m_dlights) {
            dlight.destroy(cmd_pool, logi_device);
        }
    }

    void LightManager::fill_uniform_data(U_PerFrame_InComposition& result) const {
        const auto plight_count = std::min<size_t>(dal::MAX_PLIGHT_COUNT, this->m_plights.size());
        const auto dlight_count = std::min<size_t>(dal::MAX_DLIGHT_COUNT, this->m_dlights.size());
        const auto slight_count = std::min<size_t>(dal::MAX_SLIGHT_COUNT, this->m_slights.size());

        result.m_num_of_plight_dlight_slight = glm::vec4{ plight_count, dlight_count, slight_count, 0 };

        for (size_t i = 0; i < plight_count; ++i) {
            result.m_plight_pos[i]   = glm::vec4{ this->m_plights.at(i).m_pos,   1 };
            result.m_plight_color[i] = glm::vec4{ this->m_plights.at(i).m_color, 1 };
        }

        for (size_t i = 0; i < dlight_count; ++i) {
            result.m_dlight_direc[i] = glm::vec4{ this->m_dlights.at(i).m_direc, 0 };
            result.m_dlight_color[i] = glm::vec4{ this->m_dlights.at(i).m_color, 1 };
            result.m_dlight_mat[i] = this->m_dlights.at(i).make_light_mat();
        }

        for (size_t i = 0; i < slight_count; ++i) {
            result.m_slight_pos[i]            = glm::vec4{ this->m_slights.at(i).m_pos, 0 };
            result.m_slight_direc[i]          = glm::vec4{ this->m_slights.at(i).m_direc, 0 };
            result.m_slight_color[i]          = glm::vec4{ this->m_slights.at(i).m_color, 0 };
            result.m_slight_fade_start_end[i] = glm::vec4{ this->m_slights.at(i).m_fade_start, this->m_slights.at(i).m_fade_end, 0, 0 };
        }
    }

    std::vector<VkImageView> LightManager::make_view_list(const uint32_t size) const {
        std::vector<VkImageView> result(size, this->m_dlights.at(0).m_depth_map.view());

        const auto depth_map_count = std::min<uint32_t>(size, this->m_dlights.size());
        for (uint32_t i = 0; i < depth_map_count; ++i) {
            result.at(i) = this->m_dlights.at(i).m_depth_map.view();
        }

        return result;
    }

}


// SceneNode
namespace dal {

    void SceneNode::init(const VkSurfaceKHR surface, const VkDevice logi_device, const VkPhysicalDevice phys_device) {
        this->m_cmd_pool.init(phys_device, logi_device, surface);
        this->m_desc_sets_for_dlights.init(logi_device);
    }

    void SceneNode::destroy(const VkDevice logi_device) {
        for (auto& model : this->m_models) {
            model.destroy(logi_device);
        }
        this->m_models.clear();

        this->m_lights.destroy(this->m_cmd_pool.pool(), logi_device);

        this->m_cmd_pool.destroy(logi_device);
        this->m_desc_sets_for_dlights.destroy(logi_device);
    }

    void SceneNode::on_swapchain_count_change(
        const uint32_t swapchain_count,
        const UniformBufferArray<U_PerFrame_InDeferred>& ubuf_per_frame_in_deferred,
        const VkSampler texture_sampler,
        const VkDescriptorSetLayout desc_layout_deferred,
        const VkDescriptorSetLayout desc_layout_shadow,
        const VkRenderPass renderpass_shadow,
        const VkPipeline pipeline_shadow,
        const VkPipelineLayout pipelayout_shadow,
        const VkDevice logi_device,
        const VkPhysicalDevice phys_device
    ) {
        for (auto& model : this->m_models) {
            for (auto& inst : model.instances()) {
                inst.init(swapchain_count, logi_device, phys_device);
            }

            model.reset_desc_sets(
                ubuf_per_frame_in_deferred,
                swapchain_count,
                texture_sampler,
                desc_layout_deferred,
                logi_device
            );
        }

        for (auto& dlight : this->m_lights.dlights()) {
            dlight.init(swapchain_count, renderpass_shadow, this->m_cmd_pool.pool(), logi_device, phys_device);
        }

        this->m_desc_sets_for_dlights.reset(
            this->m_models,
            this->m_lights.dlights(),
            swapchain_count,
            desc_layout_shadow,
            logi_device
        );

        for (uint32_t i = 0; i < this->m_lights.dlights().size(); ++i) {
            this->m_lights.dlights().at(i).update_cmd_buf(
                swapchain_count,
                i,
                this->m_models,
                this->m_desc_sets_for_dlights,
                renderpass_shadow,
                pipeline_shadow,
                pipelayout_shadow
            );
        }
    }

}


// Scene
namespace dal {

    void Scene::destroy(const VkDevice logi_device) {
        for (auto& node : this->m_nodes) {
            node.destroy(logi_device);
        }
        this->m_nodes.clear();
    }

}
