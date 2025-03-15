#include "first_app.hpp"
#include "ve_camera.hpp"
#include "input_controller.hpp"
#include "buffer.hpp"
#include "simple_render_system.hpp"
#include "point_light_system.hpp"
#include "outline_highlight_system.hpp"
#include "shadow_render_system.hpp"
#include "ve_imgui.hpp"
#include "utility.hpp"


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
            .setMaxSets(20000)  
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000)
            #ifdef MACOS
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            #endif
            .build();
        //Dear ImGui DescriptorPool
        imGuiPool = VeImGui::createDescriptorPool(veDevice.device());
        //load assets
        preLoadModels(veDevice);
        loadGameObjects(); 
        loadTextures();
    }
    //cleanup
    FirstApp::~FirstApp() {
        vkDestroyRenderPass(veDevice.device(), renderPass, nullptr);
        vkDestroyDescriptorPool(veDevice.device(), imGuiPool, nullptr);
        cleanupPreloadedModels();
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
            .build();
        auto textureSetLayout = VeDescriptorSetLayout::Builder(veDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3)
            .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3)
            .build();
        //create global descriptor pool
        std::vector<VkDescriptorSet> globalDescriptorSets(VeSwapChain::MAX_FRAMES_IN_FLIGHT);
        for(int i = 0; i < globalDescriptorSets.size(); i++){
            auto bufferInfo = uniformBuffers[i]->descriptorInfo();
            VeDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                .build(globalDescriptorSets[i]);
        }
        VkDescriptorSet textureDescriptorSet;
        VeDescriptorWriter(*textureSetLayout, *globalPool)
            .writeImage(0, textureInfos.data(),3)
            .writeImage(1, normalMapInfos.data(),3)
            .writeImage(2, specularMapInfos.data(),3)
            .build(textureDescriptorSet);

        //initialize render systems
        // ShadowRenderSystem shadowRenderSystem{veDevice, *globalPool };
        SimpleRenderSystem simpleRenderSystem{veDevice, veRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout(), /*shadowRenderSystem.getDescriptorSetLayout(),*/ textureSetLayout->getDescriptorSetLayout() };
        PointLightSystem pointLightSystem{veDevice, veRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
        OutlineHighlightSystem outlineHighlightSystem{veDevice, veRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
        //create camera
        VeCamera camera{};
        auto viewerObject = VeGameObject::createGameObject();
        viewerObject.transform.translation ={-1.42281f,-10.1585,0.632};
        viewerObject.transform.rotation = {-1.33747,1.56693f,0.0f};
        //camera controller
        InputController inputController{};
        //game time
        auto currentTime = std::chrono::high_resolution_clock::now();
        static float elapsedTime = 0.0f;
        //initialize selected object to control

        //initialize imgui
        renderPass = VeImGui::createRenderPass(veDevice.device(), veRenderer.getSwapChainImageFormat(), veRenderer.getSwapChainDepthFormat());
        VeImGui::createImGuiContext(veDevice, veWindow, imGuiPool, renderPass, VeSwapChain::MAX_FRAMES_IN_FLIGHT);
        int numLights = getNumLights();
        bool showOutlignHighlight = true;
        //main loop
        while (!veWindow.shouldClose()) {
            glfwPollEvents();
            //track time
            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;
            frameTime = glm::clamp(frameTime, 0.0001f, 0.1f); //clamp large frametimes
            elapsedTime += frameTime;

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
                VeImGui::initializeImGuiFrame();
                numLights = getNumLights();
                sceneEditor.drawSceneEditor(gameObjects, selectedObject, viewerObject, numLights, showOutlignHighlight);

                int frameIndex = veRenderer.getFrameIndex();
                FrameInfo frameInfo{frameIndex, frameTime, elapsedTime, commandBuffer, camera, globalDescriptorSets[frameIndex], gameObjects, selectedObject, numLights, showOutlignHighlight};
                //update global UBO
                GlobalUbo globalUbo{};
                globalUbo.projection = camera.getProjectionMatrix();
                globalUbo.view = camera.getViewMatrix();
                globalUbo.inverseView = camera.getInverseMatrix();
                globalUbo.selectedLight = selectedObject;
                globalUbo.frameTime = frameTime;     
                pointLightSystem.update(frameInfo, globalUbo);
                uniformBuffers[frameIndex]->writeToBuffer(&globalUbo);
                uniformBuffers[frameIndex]->flush();
                //render shadow maps
                // for(int i =0; i <numLights; i ++){
                //     //update shadow render system
                //     shadowRenderSystem.updateLightSpaceMatrices(frameInfo, i);
                //     //render
                //     veRenderer.beginShadowRenderPass(commandBuffer, shadowRenderSystem, frameIndex * 10 + i);
                //     shadowRenderSystem.renderGameObjects(frameInfo, i);
                //     veRenderer.endShadowRenderPass(commandBuffer, shadowRenderSystem, i);
                // }
                //render scene
                veRenderer.beginSwapChainRenderPass(commandBuffer);
                simpleRenderSystem.renderGameObjects(frameInfo, /*shadowRenderSystem.getShadowDescriptorSet(frameIndex),*/ textureDescriptorSet);
                pointLightSystem.render(frameInfo);
                if(showOutlignHighlight)
                    outlineHighlightSystem.renderGameObjects(frameInfo);
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
        auto vase = VeGameObject::createGameObject();
        vase.setTextureIndex(0);
        vase.setNormalIndex(0);
        vase.setSpecularIndex(0);
        vase.model = preLoadedModels["Cute_Demon"];
        vase.transform.translation = {0.5f, 0.5f, 0.0f};
        // vase.transform.scale = {3.0f, 1.0f, 3.0f};
        // vase.color = {128.0f, 228.1f, 229.1f}; //cyan
        vase.setTitle("Cute_Demon");
        gameObjects.emplace(vase.getId(),std::move(vase));
        //vase
        auto cube = VeGameObject::createGameObject();
        cube.setTextureIndex(1);
        cube.setNormalIndex(1);
        cube.setSpecularIndex(1);
        cube.model = preLoadedModels["cube"];
        cube.transform.translation = {1.5f, 0.5f, 0.0f};
        cube.transform.scale = {0.45f, 0.45f, 0.45f};
        // cube.color = {128.0f, 228.1f, 229.1f}; //cyan
        cube.setTitle("Cube");
        gameObjects.emplace(cube.getId(),std::move(cube));
        //object 2: floor
        auto quad = VeGameObject::createGameObject();
        quad.setTextureIndex(2);
        quad.setNormalIndex(2);
        quad.setSpecularIndex(2);
        quad.model = preLoadedModels["quad"];
        quad.transform.translation = {0.0f, 0.5f, 0.0f};
        quad.transform.scale = {3.0f, 0.5f, 3.0f};
        // quad.color = {103.0f,242.0f,209.0f};//light green
        quad.setTitle("Floor");
        gameObjects.emplace(quad.getId(),std::move(quad));

        //object 3: light
        auto light = VeGameObject::createPointLight(1.0f, .2f, {1.0f,1.0f,1.0f});
        light.setTitle("Light");
        light.transform.translation = {-0.811988f, -6.00838f, 0.1497f};
        gameObjects.emplace(light.getId(),std::move(light));
    }
    void FirstApp::loadTextures(){
        textures.push_back(std::make_unique<VeTexture>(veDevice, "textures/brick_texture.png"));
        textures.push_back(std::make_unique<VeTexture>(veDevice, "textures/metal.tga"));
        textures.push_back(std::make_unique<VeTexture>(veDevice, "textures/wood.png"));
        // textures.push_back(std::make_unique<VeTexture>(veDevice, "textures/wall_gray.png"));
        // textures.push_back(std::make_unique<VeTexture>(veDevice, "textures/tile.png"));
        // textures.push_back(std::make_unique<VeTexture>(veDevice, "textures/stone.png"));
        //normal maps
        normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/brick_normal.png"));
        normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/metal_normal.tga"));
        normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/wood_normal.png"));
        // normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/wall_gray_normal.png"));
        // normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/tile_normal.png"));
        // normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/stone_normal.png"));
        //specular maps
        specularMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/brick_specular.png"));
        specularMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/metal_specular.tga"));
        specularMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/wood_specular.png"));
        // specularMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/wall_gray_specular.png"));
        // specularMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/tile_specular.png"));
        // // specularMaps.push_back(std::make_unique<VeNormal>(veDevice, "textures/stone_specular.png"));
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
    int FirstApp::getNumLights(){
        int numLights = 0;
        for(auto& [key, object] : gameObjects){
            if(object.lightComponent!=nullptr){
                numLights++;
            }
        }
        return numLights;
    }

}