#pragma once

#include <vector>
#include <optional>

#include <vulkan/vulkan.h>

#include "vert_data.h"
#include "uniform.h"
#include "view_camera.h"
#include "command_pool.h"


namespace dal {

    class MaterialVK {

    public:
        U_Material_InDeferred m_material_data;
        UniformBuffer<U_Material_InDeferred> m_material_buffer;

        VkImageView m_albedo_map = VK_NULL_HANDLE;

    public:
        void destroy(const VkDevice logi_device);

        void set_material(
            const VkImageView texture_image_view,
            const VkDevice logi_device,
            const VkPhysicalDevice phys_device
        );

    };

    class RenderUnitVK {

    public:
        MeshBuffer m_mesh;
        MaterialVK m_material;

    public:
        void set_mesh(
            const std::vector<Vertex> vertices,
            const std::vector<uint32_t> indices,
            dal::CommandPool& cmd_pool,
            const VkDevice logi_device,
            const VkPhysicalDevice phys_device,
            const VkQueue graphics_queue
        );

    };

    class ModelInstance {

    private:
        Transform m_transform;
        UniformBufferArray<U_PerInst_PerFrame_InDeferred> m_ubuf;

    public:
        void init(const uint32_t swapchain_count, const VkDevice logi_device, const VkPhysicalDevice phys_device);
        void destroy(const VkDevice logi_device);

        void update_ubuf(const VkDevice logi_device);
        void update_ubuf(const uint32_t index, const VkDevice logi_device);

        auto& transform() {
            return this->m_transform;
        }
        auto& transform() const {
            return this->m_transform;
        }
        auto& uniform_buffers() const {
            return this->m_ubuf;
        }

    };

    class ModelVK {

    private:
        class DescSet2D {

        private:
            DescPool m_pool;
            std::vector<DescSet> m_sets;

            uint32_t m_render_unit_count = 0;
            uint32_t m_instance_count = 0;

        public:
            void init(const VkDevice logi_device);
            void destroy(const VkDevice logi_device);
            void reset(
                const std::vector<ModelInstance>& insts,
                const std::vector<RenderUnitVK>& units,
                const UniformBufferArray<U_PerFrame_InDeferred>& ubuf_per_frame_in_deferred,
                const uint32_t swapchain_count,
                const VkSampler texture_sampler,
                const VkDescriptorSetLayout desc_layout_deferred,
                const VkDevice logi_device
            );

            DescSet& at(const uint32_t swapchain_index, const uint32_t inst_index, const uint32_t unit_index);
            const DescSet& at(const uint32_t swapchain_index, const uint32_t inst_index, const uint32_t unit_index) const;


        private:
            uint32_t calc_index(const uint32_t swapchain_index, const uint32_t inst_index, const uint32_t unit_index) const;

        };

    private:
       std::vector<RenderUnitVK> m_render_units;
       std::vector<ModelInstance> m_instances;
       DescSet2D m_desc_sets;

    public:
        void init(const VkDevice logi_device) {
            this->m_desc_sets.init(logi_device);
        }
        void destroy(const VkDevice logi_device);
        void reset_desc_sets(
            const UniformBufferArray<U_PerFrame_InDeferred>& ubuf_per_frame_in_deferred,
            const uint32_t swapchain_count,
            const VkSampler texture_sampler,
            const VkDescriptorSetLayout desc_layout_deferred,
            const VkDevice logi_device
        );

        RenderUnitVK& add_unit();
        ModelInstance& add_instance(const uint32_t swapchain_count, const VkDevice logi_device, const VkPhysicalDevice phys_device);

        auto& render_units() {
            return this->m_render_units;
        }
        auto& render_units() const {
            return this->m_render_units;
        }
        auto& instances() {
            return this->m_instances;
        }
        auto& instances() const {
            return this->m_instances;
        }
        auto& desc_set(const uint32_t swapchain_index, const uint32_t inst_index, const uint32_t unit_index) const {
            return this->m_desc_sets.at(swapchain_index, inst_index, unit_index);
        }

    };


    const VkExtent2D SHADOW_MAP_EXTENT = { 1024 * 2, 1024 * 2 };

    class DepthMap {

    private:
        VkFramebuffer m_fbuf;
        FbufAttachment m_attachment;

    public:
        void init(
            const VkRenderPass renderpass_shadow,
            const VkDevice logi_device,
            const VkPhysicalDevice phys_device
        );
        void destroy(const VkDevice logi_device);

        auto& framebuffer() const {
            return this->m_fbuf;
        }
        auto& view() const {
            return this->m_attachment.view();
        }
        VkExtent2D extent() const {
            return this->m_attachment.extent();
        }

    };

    class PointLight {

    public:
        glm::vec3 m_pos;
        glm::vec3 m_color;

    };

    class DirectionalLight {

    public:
        glm::vec3 m_pos;
        glm::vec3 m_direc;
        glm::vec3 m_color;

        DepthMap m_depth_map;
        std::vector<VkCommandBuffer> m_cmd_bufs;  // For each frame
        bool m_use_shadow = false;

    public:
        void init_depth_map(
            const uint32_t swapchain_count,
            const glm::mat4& light_mat,
            const std::vector<ModelVK>& models,
            const VkRenderPass renderpass_shadow,
            const VkPipeline pipeline_shadow,
            const VkPipelineLayout pipelayout_shadow,
            const VkCommandPool cmd_pool,
            const VkDescriptorSet descset_shadow,
            const VkDevice logi_device,
            const VkPhysicalDevice phys_device
        );
        void destroy_depth_map(const VkCommandPool cmd_pool, const VkDevice logi_device);

        glm::mat4 make_light_mat() const;

    };

    class SpotLight {

    public:
        glm::vec3 m_pos;
        glm::vec3 m_direc;
        glm::vec3 m_color;
        float m_fade_start;
        float m_fade_end;

    };

    class LightManager {

    public:
        std::vector<PointLight> m_plights;
        std::vector<DirectionalLight> m_dlights;
        std::vector<SpotLight> m_slights;

    public:
        void destroy(const VkCommandPool cmd_pool, const VkDevice logi_device);

        void fill_uniform_data(U_PerFrame_InComposition& output) const;
        std::vector<VkImageView> make_view_list(const uint32_t size) const;

    };


    class SceneNode {

    public:
        std::vector<ModelVK> m_models;
        LightManager m_lights;

        CommandPool m_cmd_pool;

    public:
        void init(const VkSurfaceKHR surface, const VkDevice logi_device, const VkPhysicalDevice phys_device);
        void destroy(const VkDevice logi_device);

    };

    class Scene {

    public:
        CameraLookAt m_camera;
        std::vector<SceneNode> m_nodes;

    public:
        void destroy(const VkDevice logi_device);

    };

}
