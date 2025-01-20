#include "simple_render_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <array>
#include <stdexcept>
#include <cassert>
namespace ve {
    struct SimplePushConstantData {
        glm::mat4 transform{1.0f};
        alignas(16) glm::vec3 color;
    };

    SimpleRenderSystem::SimpleRenderSystem(VeDevice& device, VkRenderPass renderPass): veDevice{device} {
        createPipelineLayout();
        createPipeline(renderPass);
    }
    SimpleRenderSystem::~SimpleRenderSystem() {
        vkDestroyPipelineLayout(veDevice.device(), pipelineLayout, nullptr);
    }


    void SimpleRenderSystem::createPipelineLayout() {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(veDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }
    void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
        PipelineConfigInfo pipelineConfig{};
        VePipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        vePipeline = std::make_unique<VePipeline>(
            veDevice,
            "shaders/simple_shader.vert.spv",
            "shaders/simple_shader.frag.spv",
            pipelineConfig);
    }
    
    
    void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, std::vector<VeGameObject>& gameObjects, const VeCamera& camera) {
        vePipeline->bind(commandBuffer);
        auto projectionView = camera.getProjectionMatrix() * camera.getViewMatrix();
        for(auto& obj : gameObjects){
            obj.transform.rotation.y = glm::mod(obj.transform.rotation.y + 0.01f, glm::two_pi<float>());
            obj.transform.rotation.x = glm::mod(obj.transform.rotation.x + 0.001f, glm::two_pi<float>());
            SimplePushConstantData push{};
            push.color = obj.color;
            push.transform = projectionView * obj.transform.mat4();
            vkCmdPushConstants(
                commandBuffer,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(SimplePushConstantData),
                &push
            );
            obj.model->bind(commandBuffer);
            obj.model->draw(commandBuffer);
        }
    }
}