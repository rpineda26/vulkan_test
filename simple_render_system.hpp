#pragma once

#include "ve_pipeline.hpp"
#include "ve_device.hpp"
#include "ve_game_object.hpp"
#include "ve_camera.hpp"
#include "frame_info.hpp"

#include <memory>
#include <vector>
namespace ve {
    class SimpleRenderSystem{
        public:
            SimpleRenderSystem(VeDevice& device, VkRenderPass renderPass ,VkDescriptorSetLayout descriptorSetLayout);
            ~SimpleRenderSystem();
            SimpleRenderSystem(const SimpleRenderSystem&) = delete;
            SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;
            void renderGameObjects( FrameInfo& frameInfo );

        private:
            void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout);
            void createPipeline(VkRenderPass renderPass);

            VeDevice& veDevice;
            std::unique_ptr<VePipeline> vePipeline;
            VkPipelineLayout pipelineLayout;
    };
}