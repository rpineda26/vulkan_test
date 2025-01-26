#include "first_app.hpp"
#include "ve_camera.hpp"
#include "input_controller.hpp"
#include "buffer.hpp"
#include "simple_render_system.hpp"
#include "point_light_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <array>
#include <stdexcept>
#include <cassert>
#include <chrono>
#include <iostream>
namespace ve {
    struct GlobalUbo {
        glm::mat4 projection{1.0f};
        glm::mat4 view{1.0f};
        glm::mat4 inverseView{1.0f};
        glm::vec4 ambientLightColor{1.0f,1.0f,1.0f,0.1f};
        glm::vec3 lightPosition{0.0f};
        //lightPosition{-1.18f,-6.145f,-1.0255f};
        alignas(16)glm::vec4 lightColor{1.0f, 0.9f, 0.5f, 1.0f}; //white
        // lightColor{1.0f, 0.9f, 0.5f, 1.0f}; yellowish
        //lightColor{213.0f,185.0f,255.0f,1.0f}; purple
    };
    FirstApp::FirstApp() { 
        globalPool = VeDescriptorPool::Builder(veDevice)
            .setMaxSets(VeSwapChain::MAX_FRAMES_IN_FLIGHT * 10)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VeSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VeSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VeSwapChain::MAX_FRAMES_IN_FLIGHT)
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .build();
        loadGameObjects(); 
        loadTextures();
    }
    FirstApp::~FirstApp() {}

    void FirstApp::run() {
        //create global uniform buffers
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
        //bind descriptor set layout
        auto globalSetLayout = VeDescriptorSetLayout::Builder(veDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        //create global descriptor pool
        std::vector<VkDescriptorSet> globalDescriptorSets(VeSwapChain::MAX_FRAMES_IN_FLIGHT);
        for(int i = 0; i < globalDescriptorSets.size(); i++){
            auto bufferInfo = uniformBuffers[i]->descriptorInfo();
            VeDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                .writeImage(1, textureInfos[2].get())
                .writeImage(2, normalMapInfos[2].get())
                .build(globalDescriptorSets[i]);
        }
        //initialize render systems
        SimpleRenderSystem simpleRenderSystem{veDevice, veRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
        PointLightSystem pointLightSystem{veDevice, veRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
        //create camera
        VeCamera camera{};
        auto viewerObject = VeGameObject::createGameObject();
        viewerObject.transform.translation ={-1.42281f,-10.1585,0.632};
        viewerObject.transform.rotation = {-1.33747,1.56693f,0.0f};
        //camera controller
        InputController inputController{};
        glm::vec3 lightPosition{-0.811988f, -6.00838f, 0.1497f};
        //game time
        auto currentTime = std::chrono::high_resolution_clock::now();
        //initialize selected object to control
        SelectedObject selectedObject = CAMERA;
        //main loop
        while (!veWindow.shouldClose()) {
            glfwPollEvents();
            //track time
            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;
            frameTime = glm::min(frameTime, 0.1f); //clamp large frametimes

            //update objects based on input
            inputController.inputLogic(veWindow.getGLFWWindow(), frameTime, gameObjects, viewerObject, lightPosition, selectedObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);
 

            //setup viewing projection
            float aspect = veRenderer.getAspectRatio();
            // camera.setOrtho(-aspect, aspect, -0.9f, 0.9f, -1.0f, 1.0f);
            camera.setPerspective(glm::radians(45.0f), aspect, 0.1f, 1500.0f);

            //render frame
            if(auto commandBuffer = veRenderer.beginFrame()){
                int frameIndex = veRenderer.getFrameIndex();
                FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera, globalDescriptorSets[frameIndex], gameObjects};
                //update global UBO
                GlobalUbo globalUbo{};
                globalUbo.projection = camera.getProjectionMatrix();
                globalUbo.view = camera.getViewMatrix();
                globalUbo.inverseView = camera.getInverseMatrix();
                globalUbo.lightPosition = lightPosition;
                uniformBuffers[frameIndex]->writeToBuffer(&globalUbo);
                uniformBuffers[frameIndex]->flush();
                //render
                veRenderer.beginSwapChainRenderPass(commandBuffer);
                simpleRenderSystem.renderGameObjects(frameInfo);
                pointLightSystem.render(frameInfo);
                veRenderer.endSwapChainRenderPass(commandBuffer);
                veRenderer.endFrame();
            }
        }
        vkDeviceWaitIdle(veDevice.device()); //cpu wait for gpu to finish
    }
    
    void FirstApp::loadGameObjects() {
        //object 1: cube
        std::shared_ptr<VeModel> veModel = VeModel::createModelFromFile(veDevice, "models/smooth_vase.obj");
        auto vase = VeGameObject::createGameObject();
        vase.model = veModel;
        vase.transform.translation = {0.5f, 0.5f, 0.0f};
        vase.transform.scale = {3.0f, 1.0f, 3.0f};
        vase.color = {128.0f, 228.1f, 229.1f}; //cyan
        gameObjects.emplace(vase.getId(),std::move(vase));
        //vase
        veModel = VeModel::createModelFromFile(veDevice, "models/cube.obj");
        auto cube = VeGameObject::createGameObject();
        cube.model = veModel;
        cube.transform.translation = {1.5f, 0.5f, 0.0f};
        cube.transform.scale = {0.45f, 0.45f, 0.45f};
        cube.color = {128.0f, 228.1f, 229.1f}; //cyan
        gameObjects.emplace(cube.getId(),std::move(cube));
        //object 2: floor
        veModel = VeModel::createModelFromFile(veDevice, "models/quad.obj");
        auto quad = VeGameObject::createGameObject();
        quad.model = veModel;
        quad.transform.translation = {0.0f, 0.5f, 0.0f};
        quad.transform.scale = {3.0f, 0.5f, 3.0f};
        quad.color = {103.0f,242.0f,209.0f};//light green
        gameObjects.emplace(quad.getId(),std::move(quad));
    }
    void FirstApp::loadTextures(){
        textures.push_back(std::make_unique<VeTexture>(veDevice, "textures/brick_texture.png"));
        textures.push_back(std::make_unique<VeTexture>(veDevice, "textures/metal.tga"));
        textures.push_back(std::make_unique<VeTexture>(veDevice, "textures/wood.png"));
        textures.push_back(std::make_unique<VeTexture>(veDevice, "textures/wall_gray.png"));
        textures.push_back(std::make_unique<VeTexture>(veDevice, "textures/tile.png"));
        //normal maps
        normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/brick_normal.png"));
        normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/metal_normal.tga"));
        normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/wood_normal.png"));
        normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/wall_gray_normal.png"));
        normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/tile_normal.png"));
        //get image infos
        for(int i = 0; i < textures.size(); i++){
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = textures[i]->getLayout();
            imageInfo.imageView = textures[i]->getImageView();
            imageInfo.sampler = textures[i]->getSampler();
            textureInfos.push_back(std::make_unique<VkDescriptorImageInfo>(imageInfo));
            VkDescriptorImageInfo normalImageInfo{};
            normalImageInfo.imageLayout = normalMaps[i]->getLayout();
            normalImageInfo.imageView = normalMaps[i]->getNormalImageView();
            normalImageInfo.sampler = normalMaps[i]->getNormalSampler();
            normalMapInfos.push_back(std::make_unique<VkDescriptorImageInfo>(normalImageInfo)); 
        }
    }
}