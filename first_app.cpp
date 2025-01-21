#include "first_app.hpp"
#include "simple_render_system.hpp"
#include "ve_camera.hpp"
#include "input_controller.hpp"
#include "buffer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <array>
#include <stdexcept>
#include <cassert>
#include <chrono>
namespace ve {
    struct GlobalUbo {
        alignas(16)glm::mat4 projectionView{1.0f};
        alignas(16)glm::vec3 lightDirection = glm::normalize(glm::vec3{1.0f, -3.0f, -1.0f});
    };
    FirstApp::FirstApp() { 
        globalPool = VeDescriptorPool::Builder(veDevice)
            .setMaxSets(VeSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VeSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();
        loadGameObjects(); 
    }
    FirstApp::~FirstApp() {}

    void FirstApp::run() {
        std::vector<std::unique_ptr<VeBuffer>> uniformBuffers(VeSwapChain::MAX_FRAMES_IN_FLIGHT);
        for(int i =0; i< uniformBuffers.size(); i++){
            uniformBuffers[i] = std::make_unique<VeBuffer>(
                veDevice,
                sizeof(GlobalUbo),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                veDevice.properties.limits.minUniformBufferOffsetAlignment
            );
            uniformBuffers[i]->map();
        }
        auto globalSetLayout = VeDescriptorSetLayout::Builder(veDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .build();
        std::vector<VkDescriptorSet> globalDescriptorSets(VeSwapChain::MAX_FRAMES_IN_FLIGHT);
        for(int i = 0; i < globalDescriptorSets.size(); i++){
            auto bufferInfo = uniformBuffers[i]->descriptorInfo();
            VeDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                .build(globalDescriptorSets[i]);
        }

        SimpleRenderSystem simpleRenderSystem{veDevice, veRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
        VeCamera camera{};
        auto viewerObject = VeGameObject::createGameObject();
        InputController inputController{};
        auto currentTime = std::chrono::high_resolution_clock::now();

        while (!veWindow.shouldClose()) {
            glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;
            frameTime = glm::min(frameTime, 0.1f); //clamp large frametimes
            inputController.moveInPlane(veWindow.getGLFWWindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);
            //setup viewing projection
            float aspect = veRenderer.getAspectRatio();
            // camera.setOrtho(-aspect, aspect, -0.9f, 0.9f, -1.0f, 1.0f);
            camera.setPerspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);

            if(auto commandBuffer = veRenderer.beginFrame()){
                int frameIndex = veRenderer.getFrameIndex();
                FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera, globalDescriptorSets[frameIndex]};
                //update global UBO
                GlobalUbo globalUbo{};
                globalUbo.projectionView = camera.getProjectionMatrix() * camera.getViewMatrix();
                uniformBuffers[frameIndex]->writeToBuffer(&globalUbo);
                uniformBuffers[frameIndex]->flush();
                //render
                veRenderer.beginSwapChainRenderPass(commandBuffer);
                simpleRenderSystem.renderGameObjects(frameInfo,gameObjects);
                veRenderer.endSwapChainRenderPass(commandBuffer);
                veRenderer.endFrame();
            }
        }
        vkDeviceWaitIdle(veDevice.device()); //cpu wait for gpu to finish
    }
    
    void FirstApp::loadGameObjects() {
        std::shared_ptr<VeModel> lveModel = VeModel::createModelFromFile(veDevice, "models/smooth_vase.obj");
        auto cube = VeGameObject::createGameObject();
        cube.model = lveModel;
        cube.transform.translation = {0.0f, 0.0f, 2.5f};
        cube.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back(std::move(cube));
    }
}