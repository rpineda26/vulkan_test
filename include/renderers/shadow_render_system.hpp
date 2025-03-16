#pragma once

#include "ve_pipeline.hpp"
#include "ve_device.hpp"
#include "ve_game_object.hpp"
#include "ve_camera.hpp"
#include "frame_info.hpp"
#include "ve_descriptors.hpp"
#include "ve_swap_chain.hpp"
#include "buffer.hpp"  
#include <memory>
#include <vector>
namespace ve {
    class ShadowRenderSystem{
        public:
            ShadowRenderSystem(VeDevice& device, VeDescriptorPool& globalPool);
            ~ShadowRenderSystem();
            ShadowRenderSystem(const ShadowRenderSystem&) = delete;
            ShadowRenderSystem& operator=(const ShadowRenderSystem&) = delete;
            void renderGameObjects( FrameInfo& frameInfo, int lightInstance);
            VkRenderPass getRenderPass() const { return renderPass; }

            float getShadowResolution() const { return shadowResolution; }
            VkFramebuffer getFrameBuffer(int lightIndex) const { return frameBuffers[lightIndex]; }
            VkDescriptorSet getShadowDescriptorSet(int frameIndex) const { return shadowDescriptorSets[frameIndex]; }
            VkDescriptorSetLayout getDescriptorSetLayout() const { return shadowDescriptorSetLayout->getDescriptorSetLayout(); }
            VkImage getShadowImage(int lightIndex) const { return shadowImages[lightIndex]; }
            void updateLightSpaceMatrices(FrameInfo& frameInfo, int lightInstance);

        private:
            void createResources();
            void createDescriptors(VeDescriptorPool& globalPool);
            void createRenderPass();
            void createFrameBuffer();
            void createPipelineLayout();
            void createPipeline();
            void createShadowShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

            VeDevice& veDevice;
            std::unique_ptr<VePipeline> vePipeline;
            VkPipelineLayout pipelineLayout;
            VkRenderPass renderPass;
            
            std::vector<VkFramebuffer> frameBuffers;
            VkPipeline graphicsPipeline;
            VkShaderModule vertShaderModule;
    
            std::vector<VkImage> shadowImages;
            std::vector<VkImageView> shadowImageViews;
            std::vector<VkDeviceMemory> shadowImageMemories;
            std::vector<VkSampler> shadowSamplers;

            VkImageLayout shadowLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            std::vector<VkDescriptorSet> shadowDescriptorSets;
            std::vector<VkDescriptorSet> lightMatrixDescriptorSets;
            std::unique_ptr<VeDescriptorSetLayout> shadowDescriptorSetLayout;
            std::unique_ptr<VeDescriptorSetLayout> lightMatrixDescriptorSetLayout;
            std::vector<std::unique_ptr<VeBuffer>> shadowBuffers;
            float shadowResolution = 1024;
    };
}