#include "shadow_render_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <array>
#include <stdexcept>
#include <cassert>
namespace ve {
    struct ObjectConstants{
        glm::mat4 modelMatrix{1.0f};
    };
    ShadowRenderSystem::ShadowRenderSystem(
        VeDevice& device,
        VeDescriptorPool& globalPool
    ): veDevice{device} {
        createResources();
        createDescriptors(globalPool);
        createRenderPass();
        createFrameBuffer();
        createPipelineLayout();
        createPipeline();
    }
    ShadowRenderSystem::~ShadowRenderSystem() {
        vkDestroyPipeline(veDevice.device(), graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(veDevice.device(), pipelineLayout, nullptr);
        vkDestroyRenderPass(veDevice.device(), renderPass, nullptr);
        vkDestroyShaderModule(veDevice.device(), vertShaderModule, nullptr);
        for(int i = 0; i < 10; i++){
            vkDestroyFramebuffer(veDevice.device(), frameBuffers[i], nullptr);
            vkDestroyFramebuffer(veDevice.device(), frameBuffers[i + 10], nullptr);
            vkDestroySampler(veDevice.device(), shadowSamplers[i], nullptr);
            vkDestroyImageView(veDevice.device(), shadowImageViews[i], nullptr);
            vkDestroyImage(veDevice.device(), shadowImages[i], nullptr);
            vkFreeMemory(veDevice.device(), shadowImageMemories[i], nullptr);
        }
    }
    void ShadowRenderSystem::createResources(){
        int numLights = 10;
        for(int i = 0; i < numLights; i ++){
            //create Image
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = shadowResolution;
            imageInfo.extent.height = shadowResolution;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers =  6;
            imageInfo.format = VK_FORMAT_D32_SFLOAT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            VkImage shadowImage;
            VkDeviceMemory shadowImageMemory;
            veDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowImage, shadowImageMemory);
            shadowImages.push_back(shadowImage);
            shadowImageMemories.push_back(shadowImageMemory);

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = shadowImage;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            viewInfo.format = VK_FORMAT_D32_SFLOAT;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 6;
            VkImageView shadowImageView;
            if(vkCreateImageView(veDevice.device(), &viewInfo, nullptr, &shadowImageView) != VK_SUCCESS){
                throw std::runtime_error("failed to create shadow image view!");
            }
            shadowImageViews.push_back(shadowImageView);
            //sampler
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy = 1.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = 1.0f;
            VkSampler shadowSampler;
            if(vkCreateSampler(veDevice.device(), &samplerInfo, nullptr, &shadowSampler) != VK_SUCCESS){
                throw std::runtime_error("failed to create shadow sampler!");
            }
            shadowSamplers.push_back(shadowSampler);
        }
    }
    void ShadowRenderSystem::createDescriptors(VeDescriptorPool& globalPool){

        for(int i =0; i< VeSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
            auto uniformBuffers = std::make_unique<VeBuffer>(
                veDevice,
                sizeof(LightShadowUbo),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                veDevice.properties.limits.minUniformBufferOffsetAlignment
            );
            shadowBuffers.push_back(std::move(uniformBuffers));
            shadowBuffers.back()->map();
        }
        int numLights = 10;
        shadowDescriptorSetLayout = VeDescriptorSetLayout::Builder(veDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT, numLights)
            .build();
        lightMatrixDescriptorSetLayout = VeDescriptorSetLayout::Builder(veDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1)
            .build();
        std::vector<VkDescriptorImageInfo> imageInfos(numLights);
        for(int i = 0; i < numLights; i++){
        VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = shadowLayout;
            imageInfo.imageView = shadowImageViews[i];
            imageInfo.sampler = shadowSamplers[i];
            imageInfos[i] = imageInfo;
        }
        std::vector<VkDescriptorSet> sets(VeSwapChain::MAX_FRAMES_IN_FLIGHT * 2);
        for(int i =0; i <VeSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
            auto buffer = shadowBuffers[i]->descriptorInfo();
            VeDescriptorWriter(*shadowDescriptorSetLayout, globalPool)
                .writeImage(0, imageInfos.data(), numLights)
                .build(sets[i]);
            shadowDescriptorSets.push_back(sets[i]);
            VeDescriptorWriter(*lightMatrixDescriptorSetLayout, globalPool)
                .writeBuffer(0, &buffer)
                .build(sets[VeSwapChain::MAX_FRAMES_IN_FLIGHT + i]);
            lightMatrixDescriptorSets.push_back(sets[VeSwapChain::MAX_FRAMES_IN_FLIGHT + i]);
        }
    }
    void ShadowRenderSystem::createRenderPass(){
        VkAttachmentDescription shadowAttachment{};
        shadowAttachment.format = VK_FORMAT_D32_SFLOAT;
        shadowAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        shadowAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        shadowAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        shadowAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        shadowAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        shadowAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        shadowAttachment.finalLayout = shadowLayout;
        shadowAttachment.flags = 0;
        VkAttachmentReference shadowReference{};
        shadowReference.attachment = 0;
        shadowReference.layout = shadowLayout;
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 0;
        subpass.pColorAttachments = nullptr;
        subpass.pDepthStencilAttachment = &shadowReference;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = nullptr;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = nullptr;
        subpass.pResolveAttachments = nullptr;
        VkRenderPassCreateInfo shadowPassInfo{};
        shadowPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        shadowPassInfo.attachmentCount = 1;
        shadowPassInfo.pAttachments = &shadowAttachment;
        shadowPassInfo.subpassCount = 1;
        shadowPassInfo.pSubpasses = &subpass;
        shadowPassInfo.dependencyCount = 0;
        shadowPassInfo.pDependencies = nullptr;
        shadowPassInfo.flags = 0;
        if(vkCreateRenderPass(veDevice.device(), &shadowPassInfo, nullptr, &renderPass) != VK_SUCCESS){
            throw std::runtime_error("failed to create render pass!");
        }
    }
    void ShadowRenderSystem::createFrameBuffer(){
        int numLights = 10;
        for(int i = 0; i < VeSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
            for(int j = 0; j < numLights; j++){
                VkFramebuffer frameBuffer;
                VkFramebufferCreateInfo framebufferInfo{};


                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = renderPass;
                framebufferInfo.attachmentCount = 1;
                framebufferInfo.pAttachments = &shadowImageViews[j];
                framebufferInfo.width = shadowResolution;
                framebufferInfo.height = shadowResolution;
                framebufferInfo.layers = 1;
                framebufferInfo.flags = 0;
                if(vkCreateFramebuffer(veDevice.device(), &framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS){
                    throw std::runtime_error("failed to create framebuffer!");
                }
                frameBuffers.push_back(frameBuffer);
            }
        }
    }
    void ShadowRenderSystem::createPipelineLayout() {
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{lightMatrixDescriptorSetLayout->getDescriptorSetLayout()};
  
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(ObjectConstants);

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
    //shadow map pipeline should be simpler than original implementation of VePipeline class
    //will later update the VePipeline file to refactor this
    void ShadowRenderSystem::createPipeline() {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        //shader stages
        auto vertCode = VePipeline::readFile("shaders/shadow_shader.vert.spv");
        createShadowShaderModule(vertCode, &vertShaderModule);
        VkPipelineShaderStageCreateInfo vertexShaderStageInfo;
        vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStageInfo.module = vertShaderModule;
        vertexShaderStageInfo.pName = "main";
        vertexShaderStageInfo.pNext = nullptr;
        vertexShaderStageInfo.flags = 0;
        vertexShaderStageInfo.pSpecializationInfo = nullptr;
        pipelineInfo.stageCount = 1;
        pipelineInfo.pStages = &vertexShaderStageInfo;
        //vertex inpit
        std::vector<VkVertexInputBindingDescription> bindingDescriptions = VeModel::Vertex::getBindingDescriptions();
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions = VeModel::Vertex::getAttributeDescriptions();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.pNext = nullptr;
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        vertexInputInfo.flags = 0;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        //input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
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
        rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationState.depthClampEnable = VK_FALSE; 
        rasterizationState.rasterizerDiscardEnable = VK_FALSE;
        rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationState.lineWidth = 1.0f;
        rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT; // Reduce self-shadowing artifacts
        rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
        //multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;
        pipelineInfo.pMultisampleState = &multisampling;
        //pipeline layout
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        if(vkCreateGraphicsPipelines(veDevice.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS){
            throw std::runtime_error("faishaders/simple_shader.vertled to create shadow pipeline!");
        }
    }
    
    void ShadowRenderSystem::updateLightSpaceMatrices(FrameInfo& frameInfo, int lightInstance){
        // Shadow projection matrix (same for all faces)
        glm::mat4 shadowProj = glm::perspective(
            glm::radians(90.0f),  // 90 degree FOV for cube faces
            1.0f,                 // 1:1 aspect ratio
            0.1f,                 // near plane
            25.0f                 // far plane (adjust based on your light radius)
        );
        
        // Directions and up vectors for cubemap faces
        const std::array<glm::vec3, 6> directions = {
            { glm::vec3(1.0f, 0.0f, 0.0f),  // +X
            glm::vec3(-1.0f, 0.0f, 0.0f), // -X
            glm::vec3(0.0f, 1.0f, 0.0f),  // +Y
            glm::vec3(0.0f, -1.0f, 0.0f), // -Y
            glm::vec3(0.0f, 0.0f, 1.0f),  // +Z
            glm::vec3(0.0f, 0.0f, -1.0f)  // -Z
            }
        };
        
        const std::array<glm::vec3, 6> ups = {
            { glm::vec3(0.0f, -1.0f, 0.0f),  // +X
            glm::vec3(0.0f, -1.0f, 0.0f),  // -X
            glm::vec3(0.0f, 0.0f, 1.0f),   // +Y
            glm::vec3(0.0f, 0.0f, -1.0f),  // -Y
            glm::vec3(0.0f, -1.0f, 0.0f),  // +Z
            glm::vec3(0.0f, -1.0f, 0.0f)   // -Z
            }
        };

        int lightIndex = 0;
        LightShadowUbo shadowUbo{};
        for(auto& key_value : frameInfo.gameObjects){
            auto& obj = key_value.second;
            if(obj.lightComponent != nullptr){
                if(lightIndex==lightInstance){
                    for(int faceIndex = 0; faceIndex < 6; faceIndex++){
                        glm::mat4 view = glm::lookAt(
                            obj.transform.translation,
                            obj.transform.translation.x + directions[faceIndex],
                            ups[faceIndex]
                        );
                        int matrixIndex = lightIndex * 6 + faceIndex;
                        shadowUbo.pointLightShadowMap.lightSpaceMatrix[matrixIndex] = shadowProj * view;
                        shadowUbo.pointLightShadowMap.lightPosition = glm::vec4(obj.transform.translation,1.0f);
                    }
                }    
                lightIndex++;
            }
        }
        shadowUbo.numLights = lightIndex;
        shadowBuffers[0]->writeToBuffer(&shadowUbo);
        shadowBuffers[0]->flush();
    }
    
    void ShadowRenderSystem::renderGameObjects(FrameInfo& frameInfo, int lightInstance){ 
        vkCmdBindPipeline(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,
            1,
            &lightMatrixDescriptorSets[0],
            0,
            nullptr
        );
        for(auto& key_value : frameInfo.gameObjects){
            auto& obj = key_value.second;
            if(obj.lightComponent == nullptr){
                ObjectConstants objConstants{};
                objConstants.modelMatrix = obj.transform.mat4();
                vkCmdPushConstants(
                    frameInfo.commandBuffer,
                    pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT,
                    0,
                    sizeof(ObjectConstants),
                    &objConstants
                );
                obj.model->bind(frameInfo.commandBuffer);
                obj.model->drawInstanced(frameInfo.commandBuffer, 6);
            }
        }
    }
    void ShadowRenderSystem::createShadowShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule){
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        if(vkCreateShaderModule(veDevice.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS){
            throw std::runtime_error("failed to create shader module!");
        }
    }
}