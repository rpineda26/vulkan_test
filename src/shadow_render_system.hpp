#pragma once

#include "ve_pipeline.hpp"
#include "ve_device.hpp"
#include "ve_game_object.hpp"
#include "ve_camera.hpp"
#include "frame_info.hpp"

#include <memory>
#include <vector>
namespace ve {
    class ShadowRenderSystem{
        public:
            ShadowRenderSystem(VeDevice& device,VkDescriptorSetLayout descriptorSetLayout);
            ~ShadowRenderSystem();
            ShadowRenderSystem(const ShadowRenderSystem&) = delete;
            ShadowRenderSystem& operator=(const ShadowRenderSystem&) = delete;
            void renderGameObjects( FrameInfo& frameInfo );

        private:
            void createRenderPass();
            void createFrameBuffer();
            void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout);
            void createPipeline();

            VeDevice& veDevice;
            std::unique_ptr<VePipeline> vePipeline;
            VkPipelineLayout pipelineLayout;
            VkRenderPass renderPass;
            VkAttachmentDescription shadowAttachment;
            VkFrameBuffer frameBuffer;
            VkPipeline graphicsPipeline;
            VkShaderModule vertShaderModule;
    };
}