#pragma once

#include "ve_pipeline.hpp"
#include "ve_device.hpp"
#include "ve_game_object.hpp"
#include "ve_camera.hpp"
#include "frame_info.hpp"
#include "ve_descriptors.hpp"

#include <memory>
#include <vector>
namespace ve {
    class ShadowRenderSystem{
        public:
            ShadowRenderSystem(VeDevice& device,VkDescriptorSetLayout descriptorSetLayout, VeDescriptorPool& globalPool);
            ~ShadowRenderSystem();
            ShadowRenderSystem(const ShadowRenderSystem&) = delete;
            ShadowRenderSystem& operator=(const ShadowRenderSystem&) = delete;
            void renderGameObjects( FrameInfo& frameInfo );
            VkRenderPass getRenderPass() const { return renderPass; }
            VkImageView getShadowImageView() const { return shadowImageView; }
            VkSampler getShadowSampler() const { return shadowSampler; }
            VkImageLayout getShadowLayout() const { return shadowLayout; }
            float getShadowResolution() const { return shadowResolution; }
            VkFramebuffer getFrameBuffer() const { return frameBuffer; }
            VkDescriptorSet getShadowDescriptorSet() const { return shadowDescriptorSet; }
            VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

        private:
            void createResources();
            void createDescriptors(VeDescriptorPool& globalPool);
            void createRenderPass();
            void createFrameBuffer();
            void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout);
            void createPipeline();
            void createShadowShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

            VeDevice& veDevice;
            std::unique_ptr<VePipeline> vePipeline;
            VkPipelineLayout pipelineLayout;
            VkRenderPass renderPass;
            VkFramebuffer frameBuffer;
            VkPipeline graphicsPipeline;
            VkShaderModule vertShaderModule;
            VkImage shadowImage;
            VkImageView shadowImageView;
            VkSampler shadowSampler;
            VkImageLayout shadowLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            VkDeviceMemory shadowImageMemory;
            VkDescriptorSet shadowDescriptorSet;
            VkDescriptorSetLayout descriptorSetLayout;
            float shadowResolution = 1024;
    };
}