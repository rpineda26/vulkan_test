#pragma once

#include "ve_pipeline.hpp"
#include "ve_device.hpp"
#include "ve_game_object.hpp"
#include "ve_camera.hpp"

#include <memory>
#include <vector>
namespace ve {
    class SimpleRenderSystem{
        public:
            SimpleRenderSystem(VeDevice& device, VkRenderPass renderPass);
            ~SimpleRenderSystem();
            SimpleRenderSystem(const SimpleRenderSystem&) = delete;
            SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;
            void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<VeGameObject>& gameObjects, const VeCamera& camera);

        private:
            void createPipelineLayout();
            void createPipeline(VkRenderPass renderPass);

            VeDevice& veDevice;
            std::unique_ptr<VePipeline> vePipeline;
            VkPipelineLayout pipelineLayout;
    };
}