#include "first_app.hpp"
#include "ve_camera.hpp"
#include "input_controller.hpp"
#include "buffer.hpp"
#include "pbr_render_system.hpp"
#include "point_light_system.hpp"
#include "outline_highlight_system.hpp"
#include "shadow_render_system.hpp"
#include "cube_map_system.hpp"
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

        //create descriptor set layout
        //global ubo descriptor layout
        auto globalSetLayout = VeDescriptorSetLayout::Builder(veDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
            .build();
        //texture descriptor layout
        auto textureSetLayout = VeDescriptorSetLayout::Builder(veDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3)
            .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3)
            .build();
        //animation descriptor layout
        auto animationSetLayout = VeDescriptorSetLayout::Builder(veDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1)
            .build();
        
        //create descriptor pools
        //global ubo descriptor pool
        std::vector<VkDescriptorSet> globalDescriptorSets(VeSwapChain::MAX_FRAMES_IN_FLIGHT);
        std::vector<VkDescriptorSet> animationDescriptorSet(VeSwapChain::MAX_FRAMES_IN_FLIGHT);
        for(int i = 0; i < globalDescriptorSets.size(); i++){
            auto bufferInfo = uniformBuffers[i]->descriptorInfo();
            VeDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                .build(globalDescriptorSets[i]);

            auto bufferInfo2 = gameObjects.at(0).model->shaderJointsBuffer[i]->descriptorInfo();
            VeDescriptorWriter(*animationSetLayout, *globalPool)
                .writeBuffer(0,&bufferInfo2)
                .build(animationDescriptorSet[i]);
        }
        //texture descriptor pool
        VkDescriptorSet textureDescriptorSet;
        VeDescriptorWriter(*textureSetLayout, *globalPool)
            .writeImage(0, textureInfos.data(),3)
            .writeImage(1, normalMapInfos.data(),3)
            .writeImage(2, specularMapInfos.data(),3)
            .build(textureDescriptorSet);
       
        
        //initialize render systems
        // ShadowRenderSystem shadowRenderSystem{veDevice, *globalPool };
        PbrRenderSystem pbrRenderSystem{veDevice, veRenderer.getSwapChainRenderPass(), {globalSetLayout->getDescriptorSetLayout(), textureSetLayout->getDescriptorSetLayout(), animationSetLayout->getDescriptorSetLayout()/*, shadowRenderSystem.getDescriptorSetLayout()*/ } };
        PointLightSystem pointLightSystem{veDevice, veRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
        OutlineHighlightSystem outlineHighlightSystem{veDevice, veRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
        CubeMapRenderSystem cubeMapRenderSystem{veDevice, veRenderer.getSwapChainRenderPass(), {globalSetLayout->getDescriptorSetLayout(), gameObjects.at(cubeMapIndex).cubeMapComponent->descriptorSetLayout->getDescriptorSetLayout()} };
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
        int frameCount = 0;

        gameObjects.at(0).model->animationManager->start(0);
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
                //record frame data
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
                //update animation
                gameObjects.at(0).model->updateAnimation(frameTime, frameCount, frameIndex);

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
                pbrRenderSystem.renderGameObjects(frameInfo, /*shadowRenderSystem.getShadowDescriptorSet(frameIndex),*/ {globalDescriptorSets[frameIndex], textureDescriptorSet, animationDescriptorSet[frameIndex]});
                pointLightSystem.render(frameInfo);
                if(showOutlignHighlight)
                    outlineHighlightSystem.renderGameObjects(frameInfo);
                cubeMapRenderSystem.renderGameObjects(frameInfo);
                VeImGui::renderImGuiFrame(commandBuffer);
                veRenderer.endSwapChainRenderPass(commandBuffer);
                veRenderer.endFrame();
            }
            frameCount++;
        }
        vkDeviceWaitIdle(veDevice.device()); //cpu wait for gpu to finish
        VeImGui::cleanUpImGui();
    }
    
    void FirstApp::loadGameObjects() {
        // auto man = VeGameObject::createGameObject();
        // man.setTextureIndex(2);
        // man.setNormalIndex(2);
        // man.setSpecularIndex(2);
        // man.model = preLoadedModels["CesiumMan"];
        // man.transform.translation = {0.5f, 0.5f, 0.0f};
        // // man.transform.scale = {3.0f, 1.0f, 3.0f};
        // // man.color = {128.0f, 228.1f, 229.1f}; //cyan
        // man.setTitle("CesiumMan");
        // gameObjects.emplace(man.getId(),std::move(man));

        //object 1: cube
        auto vase = VeGameObject::createGameObject();
        vase.setTextureIndex(2);
        vase.setNormalIndex(2);
        vase.setSpecularIndex(2);
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
        // gameObjects.emplace(cube.getId(),std::move(cube));
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
        // gameObjects.emplace(quad.getId(),std::move(quad));

        //object 3: light
        auto light = VeGameObject::createPointLight(1.0f, .2f, {1.0f,1.0f,1.0f});
        light.setTitle("Light");
        light.transform.translation = {-0.811988f, -6.00838f, 0.1497f};
        gameObjects.emplace(light.getId(),std::move(light));

        //skybox
        auto skybox = VeGameObject::createCubeMap(veDevice, {"assets/cubemap/right.png", "assets/cubemap/left.png", "assets/cubemap/top.png", "assets/cubemap/bottom.png", "assets/cubemap/front.png", "assets/cubemap/back.png"}, *globalPool);
        skybox.setTitle("Skybox");
        gameObjects.emplace(skybox.getId(),std::move(skybox));
        cubeMapIndex = skybox.getId();
    }
    void FirstApp::loadTextures(){
        textures.push_back(std::make_unique<VeTexture>(veDevice, "assets/textures/brick_texture.png"));
        textures.push_back(std::make_unique<VeTexture>(veDevice, "assets/textures/metal.tga"));
        textures.push_back(std::make_unique<VeTexture>(veDevice, "assets/textures/wood.png"));
        // textures.push_back(std::make_unique<VeTexture>(veDevice, "assets/textures/wall_gray.png"));
        // textures.push_back(std::make_unique<VeTexture>(veDevice, "assets/textures/tile.png"));
        // textures.push_back(std::make_unique<VeTexture>(veDevice, "assets/textures/stone.png"));
        //normal maps
        normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "assets/textures/brick_normal.png"));
        normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "assets/textures/metal_normal.tga"));
        normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "assets/textures/wood_normal.png"));
        // normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "assets/textures/wall_gray_normal.png"));
        // normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "assets/textures/tile_normal.png"));
        // normalMaps.push_back(std::make_unique<VeNormal>(veDevice, "assets/textures/stone_normal.png"));
        //specular maps
        specularMaps.push_back(std::make_unique<VeNormal>(veDevice, "assets/textures/brick_specular.png"));
        specularMaps.push_back(std::make_unique<VeNormal>(veDevice, "assets/textures/metal_specular.tga"));
        specularMaps.push_back(std::make_unique<VeNormal>(veDevice, "assets/textures/wood_specular.png"));
        // specularMaps.push_back(std::make_unique<VeNormal>(veDevice, "assets/textures/wall_gray_specular.png"));
        // specularMaps.push_back(std::make_unique<VeNormal>(veDevice, "assets/textures/tile_specular.png"));
        // // specularMaps.push_back(std::make_unique<VeNormal>(veDevice, "assets/textures/stone_specular.png"));
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