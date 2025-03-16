#pragma once

#include "ve_pipeline.hpp"
#include "ve_device.hpp"
#include "ve_game_object.hpp"
#include "ve_camera.hpp"
#include "frame_info.hpp"

#include <memory>
#include <vector>
namespace ve {
    class PbrRenderSystem{
        public:
            PbrRenderSystem(VeDevice& device, VkRenderPass renderPass ,VkDescriptorSetLayout descriptorSetLayout, /*VkDescriptorSetLayout shadowSetLayout,*/ VkDescriptorSetLayout textureSetLayout);    
            ~PbrRenderSystem();
            PbrRenderSystem(const PbrRenderSystem&) = delete;
            PbrRenderSystem& operator=(const PbrRenderSystem&) = delete;
            void renderGameObjects( FrameInfo& frameInfo, /*VkDescriptorSet shadowDescriptorSet,*/ VkDescriptorSet textureDescriptorSet);

        private:
            void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout, /*VkDescriptorSetLayout shadowSetLayout,*/ VkDescriptorSetLayout textureSetLayout);
            void createPipeline(VkRenderPass renderPass);

            VeDevice& veDevice;
            std::unique_ptr<VePipeline> vePipeline;
            VkPipelineLayout pipelineLayout;
    };
}