#include "vkdevice.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <array>
#include <iostream>
#include <stdexcept>

#include "util_windows.h"
#include "model_data.h"
#include "timer.h"


namespace {

    constexpr int HALF_PROJ_BOX_LEN_OF_DLIGHT = 5;


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

    glm::mat4 make_perspective_proj_mat(const VkExtent2D extent) {
        const float ratio = static_cast<double>(extent.width) / static_cast<double>(extent.height);

        auto mat = glm::perspective<float>(glm::radians<float>(45), ratio, 0.1, 100);
        mat[1][1] *= -1;
        return mat;
    }

    glm::mat4 make_dlight_proj_mat(const float m_halfProjBoxEdgeLen) {
        auto mat = glm::ortho<float>(
            -m_halfProjBoxEdgeLen, m_halfProjBoxEdgeLen,
            -m_halfProjBoxEdgeLen, m_halfProjBoxEdgeLen,
            -m_halfProjBoxEdgeLen, m_halfProjBoxEdgeLen
        );
        mat[1][1] *= -1;
        return mat;
    }

    glm::mat4 make_dlight_view_mat(const glm::vec3 light_direc, const glm::vec3 light_pos) {
        const auto mat = glm::lookAt(-light_direc + light_pos, light_pos, glm::vec3{0, 1, 0});
        return mat;
    }

    glm::mat4 make_dlight_mat(const float half_proj_box_length, const glm::vec3 light_direc, const glm::vec3 light_pos) {
        return ::make_dlight_proj_mat(half_proj_box_length) * ::make_dlight_view_mat(light_direc, light_pos);
    }

    glm::mat4 make_dlight_mat(const float half_proj_box_length, const glm::vec4 light_direc, const glm::vec4 light_pos) {
        return ::make_dlight_mat(half_proj_box_length, glm::vec3{ light_direc }, glm::vec3{ light_pos });
    }

}


// VulkanMaster
namespace dal {

    void VulkanMaster::init(const VkInstance instance, const VkSurfaceKHR surface, const unsigned w, const unsigned h) {
        // Set member variables
        {
            this->m_currentFrame = 0;
            this->m_scrWidth = w;
            this->m_scrHeight = h;

            this->m_scene.m_camera.m_pos = glm::vec3{ 0, 2, 4 };
            auto& scene_node = this->m_scene.m_nodes.emplace_back();

            // Lights

            scene_node.m_lights.m_plights.emplace_back().m_color = glm::vec3{ 10 };
            scene_node.m_lights.m_plights.emplace_back().m_color = glm::vec4{ 100 };
            scene_node.m_lights.m_plights.emplace_back().m_color = glm::vec4{ 30 };
            scene_node.m_lights.m_plights.emplace_back().m_color = glm::vec4{ 40 };
            scene_node.m_lights.m_plights.emplace_back().m_color = glm::vec4{ 50 };

            scene_node.m_lights.m_dlights.emplace_back();
            scene_node.m_lights.m_dlights.back().m_direc = glm::normalize(glm::vec3{ 1, -2, -1 });
            scene_node.m_lights.m_dlights.back().m_color = glm::normalize(glm::vec3{ 5 });

            scene_node.m_lights.m_dlights.emplace_back();
            scene_node.m_lights.m_dlights.back().m_direc = glm::normalize(glm::vec3{ -2, -2, -1 });
            scene_node.m_lights.m_dlights.back().m_color = glm::normalize(glm::vec3{ 3 });

            scene_node.m_lights.m_dlights.emplace_back();
            scene_node.m_lights.m_dlights.back().m_direc = glm::normalize(glm::vec3{ -1, -2, 4 });
            scene_node.m_lights.m_dlights.back().m_color = glm::normalize(glm::vec3{ 2 });

            scene_node.m_lights.m_slights.emplace_back();
            scene_node.m_lights.m_slights.back().m_color = glm::vec3{ 2000 };
            scene_node.m_lights.m_slights.back().m_pos = glm::vec3{ 0, 7, -2 };
            scene_node.m_lights.m_slights.back().m_fade_start = std::cos(glm::radians<float>(45));
            scene_node.m_lights.m_slights.back().m_fade_end = std::cos(glm::radians<float>(55));
        }

        this->m_physDevice.init(instance, surface);
        this->m_logiDevice.init(surface, this->m_physDevice.get());
        this->m_swapchain.init(surface, this->m_physDevice.get(), this->m_logiDevice.get(), this->m_scrWidth, this->m_scrHeight);
        this->m_swapchainImages.init(this->m_logiDevice.get(), this->m_swapchain.get(), this->m_swapchain.imageFormat(), this->m_swapchain.extent());
        this->m_depth_image.init(this->m_swapchain.extent(), this->m_logiDevice.get(), this->m_physDevice.get());
        this->m_gbuf.init(this->m_logiDevice.get(), this->m_physDevice.get(), this->m_swapchain.extent().width, this->m_swapchain.extent().height);
        this->m_renderPass.init(this->m_logiDevice.get(), this->make_attachment_format_array());
        this->m_descSetLayout.init(this->m_logiDevice.get());
        this->m_fbuf.init(this->m_logiDevice.get(), this->m_renderPass.get(), this->m_swapchainImages.getViews(), this->m_swapchain.extent(), this->m_depth_image.image_view(), this->m_gbuf);
        this->m_depth_map_man.init(3, this->m_renderPass.shadow_mapping(), this->m_logiDevice.get(), this->m_physDevice.get());
        this->m_pipeline.init(
            this->m_logiDevice.get(),
            this->m_renderPass.get(),
            this->m_renderPass.shadow_mapping(),
            this->m_swapchain.extent(),
            this->m_depth_map_man.attachment(0).extent(),
            this->m_descSetLayout.layout_deferred(),
            this->m_descSetLayout.layout_composition(),
            this->m_descSetLayout.layout_shadow()
        );
        this->m_cmdPool.init(this->m_physDevice.get(), this->m_logiDevice.get(), surface);
        this->m_tex_man.init(this->m_logiDevice.get(), this->m_physDevice.get());

        this->load_textures();

        this->m_ubuf_per_frame_in_deferred.init(this->m_swapchainImages.size(), this->m_logiDevice.get(), this->m_physDevice.get());
        this->m_ubuf_per_frame_in_composition.init(this->m_swapchainImages.size(), this->m_logiDevice.get(), this->m_physDevice.get());
        this->m_desc_man.init(this->m_swapchainImages.size(), this->m_logiDevice.get());
        for (size_t i = 0; i < this->m_swapchainImages.size(); ++i) {
            this->m_desc_man.addSets_composition(
                this->m_logiDevice.get(),
                this->m_swapchainImages.size(),
                this->m_descSetLayout.layout_composition(),
                this->m_ubuf_per_frame_in_composition,
                this->m_gbuf.make_views_vector(this->m_depth_image.image_view()),
                this->m_depth_map_man.views(),
                this->m_tex_man.sampler_shadow_map().get()
            );
        }
        this->m_desc_man.init_descset_shadow(this->m_swapchainImages.size(), this->m_descSetLayout.layout_shadow(), this->m_logiDevice.get());

        this->m_cmdBuffers.init(this->m_logiDevice.get(), this->m_fbuf.getList().size(), this->m_cmdPool.pool());
        this->m_syncMas.init(this->m_logiDevice.get(), this->m_swapchainImages.size());

        this->load_models();

        this->m_cmdBuffers.record(
            this->m_renderPass.get(),
            this->m_pipeline.pipeline_deferred(),
            this->m_pipeline.pipeline_composition(),
            this->m_pipeline.layout_deferred(),
            this->m_pipeline.layout_composition(),
            this->m_swapchain.extent(),
            this->m_fbuf.getList(),
            this->m_desc_man.descset_composition(),
            this->m_scene.m_nodes.back().m_models
        );
        this->m_cmdBuffers.record_shadow(
            {
                this->m_scene.m_nodes.back().m_lights.m_dlights.at(0).make_light_mat(),
                this->m_scene.m_nodes.back().m_lights.m_dlights.at(1).make_light_mat(),
                this->m_scene.m_nodes.back().m_lights.m_dlights.at(2).make_light_mat(),
            },
            this->m_renderPass.shadow_mapping(),
            this->m_pipeline.pipeline_shadow(),
            this->m_pipeline.layout_shadow(),
            this->m_depth_map_man.attachment(0).extent(),
            this->m_depth_map_man.fbufs(),
            this->m_desc_man.descset_shadow().front(),
            this->m_scene.m_nodes.back().m_models
        );

        // For shadow maps transition
        this->submit_render_to_shadow_maps(this->m_cmdBuffers.shadow_map_cmd_bufs().size());
        this->waitLogiDeviceIdle();
    }

    void VulkanMaster::destroy(void) {
        this->m_scene.destroy(this->m_logiDevice.get());

        this->m_syncMas.destroy(this->m_logiDevice.get());
        //this->m_cmdBuffers.destroy(this->m_logiDevice.get(), this->m_cmdPool.pool());
        this->m_desc_man.destroy(this->m_logiDevice.get());
        this->m_ubuf_per_frame_in_composition.destroy(this->m_logiDevice.get());
        this->m_ubuf_per_frame_in_deferred.destroy(this->m_logiDevice.get());
        this->m_tex_man.destroy(this->m_logiDevice.get());

        this->m_cmdPool.destroy(this->m_logiDevice.get());
        this->m_pipeline.destroy(this->m_logiDevice.get());
        this->m_depth_map_man.destroy(this->m_logiDevice.get());
        this->m_fbuf.destroy(this->m_logiDevice.get());
        this->m_descSetLayout.destroy(this->m_logiDevice.get());
        this->m_renderPass.destroy(this->m_logiDevice.get());
        this->m_gbuf.destroy(this->m_logiDevice.get());
        this->m_depth_image.destroy(this->m_logiDevice.get());
        this->m_swapchainImages.destroy(this->m_logiDevice.get());
        this->m_swapchain.destroy(this->m_logiDevice.get());
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

        // Update uniform buffers
        this->udpate_uniform_buffers(imageIndex.first);

        // Draw shadow map
        this->submit_render_to_shadow_maps(this->m_scene.m_nodes.back().m_lights.m_dlights.size());
        this->waitLogiDeviceIdle();

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        const std::array<VkSemaphore, 1> waitSemaphores = { this->m_syncMas.semaphImageAvailable(this->m_currentFrame).get() };
        const std::array<VkPipelineStageFlags, 1> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = waitSemaphores.size();
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &this->m_cmdBuffers.buffers()[imageIndex.first];

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

        {
            this->m_syncMas.destroy(this->m_logiDevice.get());
            this->m_cmdBuffers.destroy(this->m_logiDevice.get(), this->m_cmdPool.pool());
            this->m_desc_man.destroy(this->m_logiDevice.get());
            this->m_ubuf_per_frame_in_composition.destroy(this->m_logiDevice.get());
            this->m_ubuf_per_frame_in_deferred.destroy(this->m_logiDevice.get());
            this->m_pipeline.destroy(this->m_logiDevice.get());
            this->m_fbuf.destroy(this->m_logiDevice.get());
            this->m_renderPass.destroy(this->m_logiDevice.get());
            this->m_gbuf.destroy(this->m_logiDevice.get());
            this->m_depth_image.destroy(this->m_logiDevice.get());
            this->m_swapchainImages.destroy(this->m_logiDevice.get());
            this->m_swapchain.destroy(this->m_logiDevice.get());
        }

        {
            this->m_swapchain.init(surface, this->m_physDevice.get(), this->m_logiDevice.get(), this->m_scrWidth, this->m_scrHeight);
            this->m_swapchainImages.init(this->m_logiDevice.get(), this->m_swapchain.get(), this->m_swapchain.imageFormat(), this->m_swapchain.extent());
            this->m_depth_image.init(this->m_swapchain.extent(), this->m_logiDevice.get(), this->m_physDevice.get());
            this->m_gbuf.init(this->m_logiDevice.get(), this->m_physDevice.get(), this->m_swapchain.extent().width, this->m_swapchain.extent().height);
            this->m_renderPass.init(this->m_logiDevice.get(), this->make_attachment_format_array());
            this->m_fbuf.init(this->m_logiDevice.get(), this->m_renderPass.get(), this->m_swapchainImages.getViews(), this->m_swapchain.extent(), this->m_depth_image.image_view(), this->m_gbuf);
            this->m_pipeline.init(
                this->m_logiDevice.get(),
                this->m_renderPass.get(),
                this->m_renderPass.shadow_mapping(),
                this->m_swapchain.extent(),
                this->m_depth_map_man.attachment(0).extent(),
                this->m_descSetLayout.layout_deferred(),
                this->m_descSetLayout.layout_composition(),
                this->m_descSetLayout.layout_shadow()
            );
            this->m_ubuf_per_frame_in_deferred.init(this->m_swapchainImages.size(), this->m_logiDevice.get(), this->m_physDevice.get());
            this->m_ubuf_per_frame_in_composition.init(this->m_swapchainImages.size(), this->m_logiDevice.get(), this->m_physDevice.get());
            this->m_desc_man.init(this->m_swapchainImages.size(), this->m_logiDevice.get());

            for (auto& node : this->m_scene.m_nodes) {
                for (auto& model : node.m_models) {
                    for (auto& inst : model.instances()) {
                        inst.init(this->m_swapchainImages.size(), this->m_logiDevice.get(), this->m_physDevice.get());
                    }

                    model.reset_desc_sets(
                        this->m_ubuf_per_frame_in_deferred,
                        this->m_swapchainImages.size(),
                        this->m_tex_man.sampler_1().get(),
                        this->m_descSetLayout.layout_deferred(),
                        this->m_logiDevice.get()
                    );
                }
            }

            for (size_t i = 0; i < this->m_swapchainImages.size(); ++i) {
                this->m_desc_man.addSets_composition(
                    this->m_logiDevice.get(),
                    this->m_swapchainImages.size(),
                    this->m_descSetLayout.layout_composition(),
                    this->m_ubuf_per_frame_in_composition,
                    this->m_gbuf.make_views_vector(this->m_depth_image.image_view()),
                    this->m_depth_map_man.views(),
                    this->m_tex_man.sampler_shadow_map().get()
                );
            }
            this->m_desc_man.init_descset_shadow(this->m_swapchainImages.size(), this->m_descSetLayout.layout_shadow(), this->m_logiDevice.get());

            this->m_cmdBuffers.init(this->m_logiDevice.get(), this->m_fbuf.getList().size(), this->m_cmdPool.pool());
            this->m_syncMas.init(this->m_logiDevice.get(), this->m_swapchainImages.size());
        }

        this->m_cmdBuffers.record(
            this->m_renderPass.get(),
            this->m_pipeline.pipeline_deferred(),
            this->m_pipeline.pipeline_composition(),
            this->m_pipeline.layout_deferred(),
            this->m_pipeline.layout_composition(),
            this->m_swapchain.extent(),
            this->m_fbuf.getList(),
            this->m_desc_man.descset_composition(),
            this->m_scene.m_nodes.back().m_models
        );
        this->m_cmdBuffers.record_shadow(
            {
                this->m_scene.m_nodes.back().m_lights.m_dlights.at(0).make_light_mat(),
                this->m_scene.m_nodes.back().m_lights.m_dlights.at(1).make_light_mat(),
                this->m_scene.m_nodes.back().m_lights.m_dlights.at(2).make_light_mat(),
            },
            this->m_renderPass.shadow_mapping(),
            this->m_pipeline.pipeline_shadow(),
            this->m_pipeline.layout_shadow(),
            this->m_depth_map_man.attachment(0).extent(),
            this->m_depth_map_man.fbufs(),
            this->m_desc_man.descset_shadow().front(),
            this->m_scene.m_nodes.back().m_models
        );
    }

    void VulkanMaster::load_textures() {
        // grass1
        {
            if (this->m_physDevice.does_support_astc()) {
                this->m_tex_grass = this->m_tex_man.request_texture_astc(
                    "grass1.astc",
                    this->m_cmdPool,
                    this->m_logiDevice.get(),
                    this->m_physDevice,
                    this->m_logiDevice.graphicsQ()
                );
            }
            else {
                this->m_tex_grass = this->m_tex_man.request_texture(
                    "grass1.png",
                    this->m_cmdPool,
                    this->m_logiDevice.get(),
                    this->m_physDevice,
                    this->m_logiDevice.graphicsQ()
                );
            }
        }

        // 0021di
        {
            if (this->m_physDevice.does_support_astc()) {
                const std::vector<std::string> image_names{
                    "0021di_512.astc",
                    "0021di_256.astc",
                    "0021di_128.astc",
                    "0021di_64.astc",
                    "0021di_32.astc",
                    "0021di_16.astc",
                    "0021di_8.astc",
                };

                this->m_tex_tile = this->m_tex_man.request_texture_with_mipmaps_astc(
                    image_names,
                    this->m_cmdPool,
                    this->m_logiDevice.get(),
                    this->m_physDevice,
                    this->m_logiDevice.graphicsQ()
                );
            }
            else {
                const std::vector<std::string> image_names{
                    "0021di_512.png",
                    "0021di_256.png",
                    "0021di_128.png",
                    "0021di_64.png",
                    "0021di_32.png",
                    "0021di_16.png",
                    "0021di_8.png",
                };

                this->m_tex_tile = this->m_tex_man.request_texture_with_mipmaps(
                    image_names,
                    this->m_cmdPool,
                    this->m_logiDevice.get(),
                    this->m_physDevice,
                    this->m_logiDevice.graphicsQ()
                );
            }
        }
    }

    void VulkanMaster::load_models() {
        auto& node = this->m_scene.m_nodes.back();

        // Floor
        {
            const auto mdoel_data = dal::get_horizontal_plane(500, 500);
            auto& model = node.m_models.emplace_back();
            model.init(this->m_logiDevice.get());

            auto& inst = model.add_instance(this->m_swapchainImages.size(), this->m_logiDevice.get(), this->m_physDevice.get());

            auto& unit = model.add_unit();
            unit.set_mesh(
                mdoel_data.m_vertices,
                mdoel_data.m_indices,
                this->m_cmdPool,
                this->m_logiDevice.get(),
                this->m_physDevice.get(),
                this->m_logiDevice.graphicsQ()
            );

            unit.m_material.m_material_data.m_roughness = mdoel_data.m_material.m_roughness;
            unit.m_material.m_material_data.m_metallic = mdoel_data.m_material.m_metallic;

            unit.m_material.set_material(
                this->m_tex_grass->view.get(),
                this->m_logiDevice.get(),
                this->m_physDevice.get()
            );

            model.reset_desc_sets(
                this->m_ubuf_per_frame_in_deferred,
                this->m_swapchainImages.size(),
                this->m_tex_man.sampler_1().get(),
                this->m_descSetLayout.layout_deferred(),
                this->m_logiDevice.get()
            );
        }

        // Box
        {
            const auto model_data = dal::get_aabb_box();

            auto& model = node.m_models.emplace_back();
            model.init(this->m_logiDevice.get());

            auto& inst = model.add_instance(this->m_swapchainImages.size(), this->m_logiDevice.get(), this->m_physDevice.get());
            inst.transform().m_pos.x = 2;
            inst.update_ubuf(this->m_logiDevice.get());

            auto& unit = model.add_unit();
            unit.set_mesh(
                model_data.m_vertices,
                model_data.m_indices,
                this->m_cmdPool,
                this->m_logiDevice.get(),
                this->m_physDevice.get(),
                this->m_logiDevice.graphicsQ()
            );

            unit.m_material.m_material_data.m_roughness = model_data.m_material.m_roughness;
            unit.m_material.m_material_data.m_metallic = model_data.m_material.m_metallic;

            unit.m_material.set_material(
                this->m_tex_tile->view.get(),
                this->m_logiDevice.get(),
                this->m_physDevice.get()
            );

            model.reset_desc_sets(
                this->m_ubuf_per_frame_in_deferred,
                this->m_swapchainImages.size(),
                this->m_tex_man.sampler_1().get(),
                this->m_descSetLayout.layout_deferred(),
                this->m_logiDevice.get()
            );
        }

        // Yuri
        {
            auto& model = node.m_models.emplace_back();
            model.init(this->m_logiDevice.get());

            auto& inst1 = model.add_instance(this->m_swapchainImages.size(), this->m_logiDevice.get(), this->m_physDevice.get());
            inst1.transform().m_scale = 0.02;
            inst1.update_ubuf(this->m_logiDevice.get());

            auto& inst2 = model.add_instance(this->m_swapchainImages.size(), this->m_logiDevice.get(), this->m_physDevice.get());
            inst2.transform().m_scale = 0.02;
            inst2.transform().m_pos.x = -1;
            inst2.update_ubuf(this->m_logiDevice.get());

            for (const auto& model_data : get_test_model()) {
                auto& unit = model.add_unit();

                unit.set_mesh(
                    model_data.m_vertices,
                    model_data.m_indices,
                    this->m_cmdPool,
                    this->m_logiDevice.get(),
                    this->m_physDevice.get(),
                    this->m_logiDevice.graphicsQ()
                );

                const auto& tex = this->m_tex_man.request_texture(
                    model_data.m_material.m_albedo_map.c_str(),
                    this->m_cmdPool,
                    this->m_logiDevice.get(),
                    this->m_physDevice,
                    this->m_logiDevice.graphicsQ()
                );

                unit.m_material.m_material_data.m_roughness = model_data.m_material.m_roughness;
                unit.m_material.m_material_data.m_metallic = model_data.m_material.m_metallic;

                unit.m_material.set_material(
                    tex->view.get(),
                    this->m_logiDevice.get(),
                    this->m_physDevice.get()
                );

                model.reset_desc_sets(
                    this->m_ubuf_per_frame_in_deferred,
                    this->m_swapchainImages.size(),
                    this->m_tex_man.sampler_1().get(),
                    this->m_descSetLayout.layout_deferred(),
                    this->m_logiDevice.get()
                );
            }
        }

        // Irin
        {
            auto& model = node.m_models.emplace_back();
            model.init(this->m_logiDevice.get());

            auto& inst = model.add_instance(this->m_swapchainImages.size(), this->m_logiDevice.get(), this->m_physDevice.get());
            inst.transform().m_scale = 0.8;
            inst.transform().m_pos.x = -2;
            inst.update_ubuf(this->m_logiDevice.get());

            for (const auto& model_data : load_dmd_model("irin.dmd")) {
                auto& unit = model.add_unit();

                unit.set_mesh(
                    model_data.m_vertices,
                    model_data.m_indices,
                    this->m_cmdPool,
                    this->m_logiDevice.get(),
                    this->m_physDevice.get(),
                    this->m_logiDevice.graphicsQ()
                );

                const auto& tex = this->m_tex_man.request_texture(
                    model_data.m_material.m_albedo_map.c_str(),
                    this->m_cmdPool,
                    this->m_logiDevice.get(),
                    this->m_physDevice,
                    this->m_logiDevice.graphicsQ()
                );

                unit.m_material.m_material_data.m_roughness = model_data.m_material.m_roughness;
                unit.m_material.m_material_data.m_metallic = model_data.m_material.m_metallic;

                unit.m_material.set_material(
                    tex->view.get(),
                    this->m_logiDevice.get(),
                    this->m_physDevice.get()
                );

                model.reset_desc_sets(
                    this->m_ubuf_per_frame_in_deferred,
                    this->m_swapchainImages.size(),
                    this->m_tex_man.sampler_1().get(),
                    this->m_descSetLayout.layout_deferred(),
                    this->m_logiDevice.get()
                );
            }
        }
    }

    void VulkanMaster::notifyScreenResize(const unsigned w, const unsigned h) {
        this->m_needResize = true;
        this->m_scrWidth = w;
        this->m_scrHeight = h;
    }

    void VulkanMaster::submit_render_to_shadow_maps(const int work_count) {
        VkPipelineStageFlags shadow_map_wait_stages = 0;
        VkSubmitInfo submit_info{ };
        submit_info.pNext = NULL;
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = NULL;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;
        submit_info.pWaitDstStageMask = 0;
        submit_info.commandBufferCount = 1;

        for (int i = 0; i < work_count; ++i) {
            submit_info.pCommandBuffers = &this->m_cmdBuffers.shadow_map_cmd_bufs().at(i);

            const auto submit_result = vkQueueSubmit(this->m_logiDevice.graphicsQ(), 1, &submit_info, nullptr);
            if ( submit_result != VK_SUCCESS ) {
                throw std::runtime_error("failed to submit draw command buffer!");
            }
        }
    }

    void VulkanMaster::udpate_uniform_buffers(const uint32_t swapchain_index) {
        {
            U_PerFrame_InDeferred data_per_frame_in_deferred;
            data_per_frame_in_deferred.proj = ::make_perspective_proj_mat(this->m_swapchain.extent());
            data_per_frame_in_deferred.view = this->camera().make_view_mat();

            this->m_ubuf_per_frame_in_deferred.copy_to_buffer(
                swapchain_index,
                data_per_frame_in_deferred,
                this->m_logiDevice.get()
            );
        }

        {
            constexpr double RADIUS = 5;

            const auto plight_count = this->m_scene.m_nodes.back().m_lights.m_plights.size();
            for (size_t i = 0; i < plight_count; ++i) {
                const auto rotate_phase_diff = 2.0 * M_PI / static_cast<double>(plight_count);
                this->m_scene.m_nodes.back().m_lights.m_plights.at(i).m_pos = glm::vec4{
                    RADIUS * std::cos(dal::getTimeInSec() + rotate_phase_diff * i),
                    4,
                    RADIUS * std::sin(dal::getTimeInSec() + rotate_phase_diff * i),
                    1
                };
            }
        }

        {
            auto& yuri_model = this->m_scene.m_nodes.back().m_models.at(2);
            auto& yuri_inst = yuri_model.instances().at(0);

            yuri_inst.transform().m_pos = glm::vec3{
                std::cos(dal::getTimeInSec()),
                0,
                std::sin(dal::getTimeInSec()) - 3
            };
            yuri_inst.update_ubuf(swapchain_index, this->m_logiDevice.get());
        }

        this->m_scene.m_nodes.back().m_lights.m_slights.at(0).m_direc = glm::normalize(glm::vec3{
            std::sin(dal::getTimeInSec()),
            std::cos(dal::getTimeInSec()),
            -0.4
        });

        U_PerFrame_InComposition data;
        data.m_view_pos = glm::vec4{ this->camera().m_pos, 1 };
        this->m_scene.m_nodes.back().m_lights.fill_uniform_data(data);
        this->m_ubuf_per_frame_in_composition.copy_to_buffer(swapchain_index, data, this->m_logiDevice.get());
    }

}
