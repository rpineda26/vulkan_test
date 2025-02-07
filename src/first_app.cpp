#include "first_app.hpp"
#include "ve_camera.hpp"
#include "input_controller.hpp"
#include "buffer.hpp"
#include "simple_render_system.hpp"
#include "point_light_system.hpp"
#include "ve_imgui.hpp"


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
    FirstApp::FirstApp() { 
        //setup descriptor pools
        globalPool = VeDescriptorPool::Builder(veDevice)
            .setMaxSets(VeSwapChain::MAX_FRAMES_IN_FLIGHT * 15 + 2)  
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VeSwapChain::MAX_FRAMES_IN_FLIGHT) //CAMERA INFO
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VeSwapChain::MAX_FRAMES_IN_FLIGHT * 5) //ALBEDO ARRAY
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VeSwapChain::MAX_FRAMES_IN_FLIGHT * 5) //NORMAL MAP ARRAY
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VeSwapChain::MAX_FRAMES_IN_FLIGHT * 5) //SPECULAR MAP ARRAY
            #ifdef MACOS
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            #endif
            .build();
        //Dear ImGui DescriptorPool
        imGuiPool = VeImGui::createDescriptorPool(veDevice.device());
        //load assets
        loadGameObjects(); 
        loadTextures();
    }
    //cleanup
    FirstApp::~FirstApp() {
        vkDestroyRenderPass(veDevice.device(), renderPass, nullptr);
        vkDestroyDescriptorPool(veDevice.device(), imGuiPool, nullptr);
    }

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
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5)
            .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5)
            .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5)
            .build();

        //create global descriptor pool
        std::vector<VkDescriptorSet> globalDescriptorSets(VeSwapChain::MAX_FRAMES_IN_FLIGHT);
        for(int i = 0; i < globalDescriptorSets.size(); i++){
            auto bufferInfo = uniformBuffers[i]->descriptorInfo();
            VeDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                .writeImage(1, textureInfos.data(),5)
                .writeImage(2, normalMapInfos.data(),5)
                .writeImage(3, specularMapInfos.data(),5)
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
        //game time
        auto currentTime = std::chrono::high_resolution_clock::now();
        //initialize selected object to control

        //initialize imgui
        renderPass = VeImGui::createRenderPass(veDevice.device(), veRenderer.getSwapChainImageFormat(), veRenderer.getSwapChainDepthFormat());
        VeImGui::createImGuiContext(veDevice, veWindow, imGuiPool, renderPass, VeSwapChain::MAX_FRAMES_IN_FLIGHT);
    
        //main loop
        while (!veWindow.shouldClose()) {
            glfwPollEvents();
            //track time
            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;
            frameTime = glm::min(frameTime, 0.1f); //clamp large frametimes

            //update objects based on input
            inputController.inputLogic(veWindow.getGLFWWindow(), frameTime, gameObjects, viewerObject, selectedObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);
 
            //setup viewing projection
            float aspect = veRenderer.getAspectRatio();
            // camera.setOrtho(-aspect, aspect, -0.9f, 0.9f, -1.0f, 1.0f);
            camera.setPerspective(glm::radians(45.0f), aspect, 0.1f, 1500.0f);
            
            //render frame
            if(auto commandBuffer = veRenderer.beginFrame()){
                //start new imgui frame
                VeImGui::initializeImGuiFrame();;
                sceneEditor.drawSceneEditor(gameObjects, selectedObject, viewerObject);

                int frameIndex = veRenderer.getFrameIndex();
                FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera, globalDescriptorSets[frameIndex], gameObjects};
                //update global UBO
                GlobalUbo globalUbo{};
                globalUbo.projection = camera.getProjectionMatrix();
                globalUbo.view = camera.getViewMatrix();
                globalUbo.inverseView = camera.getInverseMatrix();
                pointLightSystem.update(frameInfo, globalUbo);
                uniformBuffers[frameIndex]->writeToBuffer(&globalUbo);
                uniformBuffers[frameIndex]->flush();
                //render
                veRenderer.beginSwapChainRenderPass(commandBuffer);
                simpleRenderSystem.renderGameObjects(frameInfo);
                pointLightSystem.render(frameInfo);
                VeImGui::renderImGuiFrame(commandBuffer);
                veRenderer.endSwapChainRenderPass(commandBuffer);
                veRenderer.endFrame();
            }
        }
        vkDeviceWaitIdle(veDevice.device()); //cpu wait for gpu to finish
        VeImGui::cleanUpImGui();
    }
    
    void FirstApp::loadGameObjects() {
        //object 1: cube
        std::shared_ptr<VeModel> veModel = VeModel::createModelFromFile(veDevice, "models/smooth_vase.obj");
        veModel->setTextureIndex(1);
        veModel->setNormalIndex(1);
        veModel->setSpecularIndex(1);
        auto vase = VeGameObject::createGameObject();
        vase.model = veModel;
        vase.transform.translation = {0.5f, 0.5f, 0.0f};
        vase.transform.scale = {3.0f, 1.0f, 3.0f};
        vase.color = {128.0f, 228.1f, 229.1f}; //cyan
        vase.setTitle("Vase");
        gameObjects.emplace(vase.getId(),std::move(vase));
        //vase
        veModel = VeModel::createModelFromFile(veDevice, "models/cube.obj");
        veModel->setTextureIndex(0);
        veModel->setNormalIndex(0);
        veModel->setSpecularIndex(0);
        auto cube = VeGameObject::createGameObject();
        cube.model = veModel;
        cube.transform.translation = {1.5f, 0.5f, 0.0f};
        cube.transform.scale = {0.45f, 0.45f, 0.45f};
        cube.color = {128.0f, 228.1f, 229.1f}; //cyan
        cube.setTitle("Cube");
        gameObjects.emplace(cube.getId(),std::move(cube));
        //object 2: floor
        veModel = VeModel::createModelFromFile(veDevice, "models/quad.obj");
        veModel->setTextureIndex(3);
        veModel->setNormalIndex(3);
        veModel->setSpecularIndex(3);
        auto quad = VeGameObject::createGameObject();
        quad.model = veModel;
        quad.transform.translation = {0.0f, 0.5f, 0.0f};
        quad.transform.scale = {3.0f, 0.5f, 3.0f};
        quad.color = {103.0f,242.0f,209.0f};//light green
        quad.setTitle("Floor");
        gameObjects.emplace(quad.getId(),std::move(quad));

        //object 3: light
        auto light = VeGameObject::createPointLight(1.0f, .2f, {1.0f,1.0f,1.0f});
        light.transform.translation = {-0.811988f, -6.00838f, 0.1497f};
        gameObjects.emplace(light.getId(),std::move(light));
    }
    void FirstApp::loadTextures(){
        textures.push_back(std::make_unique<VeTexture>(veDevice, "textures/brick_texture.png"));
        textures.push_back(std::make_unique<VeTexture>(veDevice, "textures/metal.tga"));
        textures.push_back(std::make_unique<VeTexture>(veDevice, "textures/wood.png"));
        textures.push_back(std::make_unique<VeTexture>(veDevice, "textures/wall_gray.png"));
        textures.push_back(std::make_unique<VeTexture>(veDevice, "textures/tile.png"));
        // textures.push_back(std::make_unique<VeTexture>(veDevice, "textures/stone.png"));
        //normal maps
        normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/brick_normal.png"));
        normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/metal_normal.tga"));
        normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/wood_normal.png"));
        normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/wall_gray_normal.png"));
        normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/tile_normal.png"));
        // normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/stone_normal.png"));
        //specular maps
        specularMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/brick_specular.png"));
        specularMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/metal_specular.tga"));
        specularMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/wood_specular.png"));
        specularMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/wall_gray_specular.png"));
        specularMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/tile_specular.png"));
        // specularMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/stone_specular.png"));
        //get image infos
        for(int i = 0; i < textures.size(); i++){
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = textures[i]->getLayout();
            imageInfo.imageView = textures[i]->getImageView();
            imageInfo.sampler = textures[i]->getSampler();
            textureInfos.push_back(VkDescriptorImageInfo(imageInfo));
            VkDescriptorImageInfo normalImageInfo{};
            normalImageInfo.imageLayout = normalMaps[i]->getLayout();
            normalImageInfo.imageView = normalMaps[i]->getNormalImageView();
            normalImageInfo.sampler = normalMaps[i]->getNormalSampler();
            normalMapInfos.push_back(VkDescriptorImageInfo(normalImageInfo)); 
            VkDescriptorImageInfo specularImageInfo{};
            specularImageInfo.imageLayout = specularMaps[i]->getLayout();
            specularImageInfo.imageView = specularMaps[i]->getNormalImageView();
            specularImageInfo.sampler = specularMaps[i]->getNormalSampler();
            specularMapInfos.push_back(VkDescriptorImageInfo(specularImageInfo));
        }
    }

}