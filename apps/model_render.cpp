#include "model_render.h"

#include <stdexcept>


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


namespace dal {

    void RenderUnitVK::set_mesh(
        const std::vector<Vertex> vertices,
        const std::vector<uint32_t> indices,
        dal::CommandPool& cmd_pool,
        const VkDevice logi_device,
        const VkPhysicalDevice phys_device,
        const VkQueue graphics_queue
    ) {
        this->m_mesh.vertices.init(vertices, logi_device, phys_device, cmd_pool, graphics_queue);
        this->m_mesh.indices.init(indices, logi_device, phys_device, cmd_pool, graphics_queue);
    }

}


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


namespace dal {

    void ModelVK::DescSet2D::init(const VkDevice logi_device) {
        this->m_pool.init(128, 128, 128, 128, logi_device);
    }

    void ModelVK::DescSet2D::destroy(const VkDevice logi_device) {
        this->m_pool.destroy(logi_device);
        this->m_sets.clear();

        this->m_render_unit_count = 0;
        this->m_instance_count = 0;
    }

    DescSet& ModelVK::DescSet2D::at(const uint32_t swapchain_index, const uint32_t inst_index, const uint32_t unit_index) {
        return this->m_sets.at(this->calc_index(swapchain_index, inst_index, unit_index));
    }
    const DescSet& ModelVK::DescSet2D::at(const uint32_t swapchain_index, const uint32_t inst_index, const uint32_t unit_index) const {
        return this->m_sets.at(this->calc_index(swapchain_index, inst_index, unit_index));
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
        this->m_render_unit_count = units.size();
        this->m_instance_count = insts.size();

        this->m_pool.reset(logi_device);
        this->m_sets.resize(swapchain_count * insts.size() * units.size());

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

    uint32_t ModelVK::DescSet2D::calc_index(const uint32_t swapchain_index, const uint32_t inst_index, const uint32_t unit_index) const {
        return (this->m_render_unit_count * this->m_instance_count * swapchain_index) + (this->m_render_unit_count * inst_index) + unit_index;
    }

}


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


namespace dal {

    void DepthMap::init(
        const uint32_t count,
        const VkRenderPass render_pass,
        const VkDevice logi_device,
        const VkPhysicalDevice phys_device
    ) {
        assert(0 != count);
        assert(VK_NULL_HANDLE != render_pass);
        assert(VK_NULL_HANDLE != logi_device);
        assert(VK_NULL_HANDLE != phys_device);

        this->destroy(logi_device);
        this->m_attachment.init(logi_device, phys_device, VK_FORMAT_D32_SFLOAT, FbufAttachment::Usage::depth_map, 1024*4, 1024*4);

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = render_pass;
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


    glm::mat4 DirectionalLight::make_light_mat() const {
        constexpr float half_proj_box_edge_length = 10;

        const auto view_mat = glm::lookAt(-this->m_direc + this->m_pos, this->m_pos, glm::vec3{0, 1, 0});

        auto proj_mat = glm::ortho<float>(
            -half_proj_box_edge_length, half_proj_box_edge_length,
            -half_proj_box_edge_length, half_proj_box_edge_length,
            -half_proj_box_edge_length, half_proj_box_edge_length
        );
        proj_mat[1][1] *= -1;

        return proj_mat * view_mat;
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

}


namespace dal {

    void SceneNode::destroy(const VkDevice logi_device) {
        for (auto& model : this->m_models) {
            model.destroy(logi_device);
        }
        this->m_models.clear();
    }

    void Scene::destroy(const VkDevice logi_device) {
        for (auto& node : this->m_nodes) {
            node.destroy(logi_device);
        }
        this->m_nodes.clear();
    }

}