#include "shadow_render_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <array>
#include <stdexcept>
#include <cassert>
namespace ve {
    ShadowRenderSystem::ShadowRenderSystem(
        VeDevice& device, VkDescriptorSetLayout globalSetLayout
    ): veDevice{device} {
        createRenderPass();
        createFrameBuffer();
        createPipelineLayout(globalSetLayout);
        createPipeline(renderPass);
    }
    ShadowRenderSystem::~ShadowRenderSystem() {
        vkDestroyPipelineLayout(veDevice.device(), pipelineLayout, nullptr);
        vkDestroyShaderModule(veDevice.device(), vertShaderModule, nullptr);
        vkDestroyPipeline(veDevice.device(), graphicsPipeline, nullptr);
        vkDestroyRenderPass(veDevice.device(), renderPass, nullptr);
    }

    void ShadowRenderSystem::createRenderPass(){
        shadowAttachment.format = VK_FORMAT_D32_SFLOAT;
        shadowAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        shadowAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        shadowAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        shadowAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        shadowAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        shadowAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        if(vkCreateRenderPass(veDevice.device(), &shadowAttachment, nullptr, &renderPass) != VK_SUCCESS){
            throw std::runtime_error("failed to create render pass!");
        }
    }
    void ShadowRenderSystem::createFrameBuffer(){
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &shadowAttachment;
        framebufferInfo.width = 1024;
        framebufferInfo.height = 1024;
        framebufferInfo.layers = 1;
        if(vkCreateFramebuffer(veDevice.device(), &framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS){
            throw std::runtime_error("failed to create framebuffer!");
        }
        
    }
    void ShadowRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        if (vkCreatePipelineLayout(veDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }
    //shadow map pipeline should be simpler than original implementation of VePipeline class
    //will later update the VePipeline file to refactor this
    void ShadowRenderSystem::createPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        //shader stages
        auto vertCode = readFile("shaders/shadow_shader.vert.spv");
        createShaderModule(vertCode, &vertShaderModule);
        VkPipelineShaderStageCreateInfo vertexShader;
        vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStageInfo.module = vertCode;
        vertexShaderStageInfo.pName = "main";
        pipelineInfo.stageCount = 1;
        pipelineInfo.pStages = &vertexShaderStageInfo;
        //vertex inpit
        auto& bindingDescriptions = VeModel::Vertex::getBindingDescriptions();
        auto& attributeDescriptions = VeModel::Vertex::getAttributeDescriptions();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        //input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemly.primitiveRestartEnable = VK_FALSE;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        //viewport
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;
        pipelineInfo.pViewportState = &viewportState;
        //rastrization
        //enable depth bias
        VkPipelineRasterizationStateCreateInfo rasterizationState{};
        rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_TRUE; // Prevents shadow map clipping
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT; // Reduce self-shadowing artifacts
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationState.depthBiasEnable = VK_TRUE;
        rasterizationState.depthBiasConstantFactor = 1.25f;
        rasterizationState.depthBiasSlopeFactor = 1.75f;
        pipelineInfo.pRasterizationState = &rasterizationState;
        //depth& stencil depth only
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
        pipelineInfo.pDepthStencilState = &depthStencil;
        //disable color blending
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.attachmentCount = 0;
        pipelineInfo.pColorBlendState = &colorBlending;
        //dynamic states
        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;
        pipelineInfo.pDynamicState = &dynamicState;
        //pipeline layout
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        if(vkCreateGraphicsPipelines(veDevice.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS){
            throw std::runtime_error("failed to create shadow pipeline!");
        }
    }
    
    
    void ShadowRenderSystem::renderGameObjects(FrameInfo& frameInfo) {
        vePipeline->bind(frameInfo.commandBuffer);
        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,
            1,
            &frameInfo.descriptorSet,
            0,
            nullptr
        );
        for(auto& key_value : frameInfo.gameObjects){
            auto& obj = key_value.second;
            if(obj.lightComponent == nullptr){
                SimplePushConstantData push{};
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
                    sizeof(SimplePushConstantData),
                    &push
                );
                obj.model->bind(frameInfo.commandBuffer);
                obj.model->draw(frameInfo.commandBuffer);
            }
        }
    }
}