#include "pbr_render_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <array>
#include <stdexcept>
#include <cassert>
namespace ve {
    struct PbrPushConstantData {
        glm::mat4 modelMatrix{1.0f};
        glm::mat4 normalMatrix{1.0f};
        uint32_t textureIndex{0};
        uint32_t normalIndex{0};
        uint32_t specularIndex{0};
        float smoothness{0.0f};
        glm::vec3 baseColor{1.0f};
    };

    PbrRenderSystem::PbrRenderSystem(
        VeDevice& device, VkRenderPass renderPass, 
        VkDescriptorSetLayout globalSetLayout,
        /*VkDescriptorSetLayout shadowSetLayout,*/
        VkDescriptorSetLayout textureSetLayout
    ): veDevice{device} {
        createPipelineLayout(globalSetLayout, /*shadowSetLayout,*/ textureSetLayout);
        createPipeline(renderPass);
    }
    PbrRenderSystem::~PbrRenderSystem() {
        vkDestroyPipelineLayout(veDevice.device(), pipelineLayout, nullptr);
    }


    void PbrRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout, /*VkDescriptorSetLayout shadowSetLayout,*/ VkDescriptorSetLayout textureSetLayout) {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PbrPushConstantData);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout, /*shadowSetLayout,*/ textureSetLayout};
   
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(veDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }
    void PbrRenderSystem::createPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
        PipelineConfigInfo pipelineConfig{};
        VePipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        vePipeline = std::make_unique<VePipeline>(
            veDevice,
            "shaders/pbr_shader.vert.spv",
            "shaders/pbr_shader.frag.spv",
            pipelineConfig);
    }
    
    
    void PbrRenderSystem::renderGameObjects(FrameInfo& frameInfo, /*VkDescriptorSet shadowDescriptorSet,*/ VkDescriptorSet textureDescriptorSet) {
        vePipeline->bind(frameInfo.commandBuffer);
        std::array<VkDescriptorSet, 2> descriptorSets = {frameInfo.descriptorSet, /*shadowDescriptorSet,*/ textureDescriptorSet};
 
        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,  // First set index
            static_cast<uint32_t>(descriptorSets.size()), // Number of sets
            descriptorSets.data(), // Pointer to descriptor sets
            0,
            nullptr
        );
        for(auto& key_value : frameInfo.gameObjects){
            auto& obj = key_value.second;
            if(obj.lightComponent == nullptr){
                PbrPushConstantData push{};
                push.modelMatrix =  obj.transform.mat4();
                push.normalMatrix = obj.transform.normalMatrix();
                push.textureIndex = obj.getTextureIndex();
                push.normalIndex = obj.getNormalIndex();
                push.specularIndex = obj.getSpecularIndex();
                push.smoothness = obj.getSmoothness();
                push.baseColor = obj.color;
                
                vkCmdPushConstants(
                    frameInfo.commandBuffer,
                    pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(PbrPushConstantData),
                    &push
                );
                obj.model->bind(frameInfo.commandBuffer);
                obj.model->draw(frameInfo.commandBuffer);
            }
        }
    }
}