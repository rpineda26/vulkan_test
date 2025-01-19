#include "first_app.hpp"
#include <stdexcept>
namespace ve {
    FirstApp::FirstApp() {
        createPipelineLayout();
        createPipeline();
        createCommandBuffers();
    }
    FirstApp::~FirstApp() {
        vkDestroyPipelineLayout(veDevice.device(), pipelineLayout, nullptr);
    }

    void FirstApp::run() {
        while (!veWindow.shouldClose()) {
            glfwPollEvents();
        }
    }
    void FirstApp::createPipelineLayout() {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        if (vkCreatePipelineLayout(veDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }
    void FirstApp::createPipeline() {
        auto pipelineConfig = VePipeline::defaultPipelineConfigInfo(veSwapChain.width(), veSwapChain.height());
        pipelineConfig.renderPass = veSwapChain.getRenderPass();
        pipelineConfig.pipelineLayout = pipelineLayout;
        vePipeline = std::make_unique<VePipeline>(
            veDevice,
            "shaders/simple_shader.vert.spv",
            "shaders/simple_shader.frag.spv",
            pipelineConfig);
    }
    void FirstApp::createCommandBuffers() {}
    void FirstApp::drawFrame(){}
}