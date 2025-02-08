#pragma once

#include "ve_pipeline.hpp"
#include "ve_device.hpp"
#include "ve_game_object.hpp"
#include "ve_camera.hpp"
#include "frame_info.hpp"

#include <memory>
#include <vector>
namespace ve {
    class OutlineHighlightSystem{
        public:
            OutlineHighlightSystem(VeDevice& device, VkRenderPass renderPass ,VkDescriptorSetLayout descriptorSetLayout);
            ~OutlineHighlightSystem();
            OutlineHighlightSystem(const OutlineHighlightSystem&) = delete;
            OutlineHighlightSystem& operator=(const OutlineHighlightSystem&) = delete;
            void renderGameObjects( FrameInfo& frameInfo, int selectedObject);

        private:
            void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout);
            void createPipeline(VkRenderPass renderPass);

            VeDevice& veDevice;
            std::unique_ptr<VePipeline> vePipeline;
            VkPipelineLayout pipelineLayout;
    };
}