#include "first_app.hpp"
#include "ve_camera.hpp"
#include "input_controller.hpp"
#include "buffer.hpp"
#include "simple_render_system.hpp"
#include "point_light_system.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

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
        alignas(16)glm::vec4 lightColor{1.0f}; //white
        // lightColor{1.0f, 0.9f, 0.5f, 1.0f}; yellowish
        //lightColor{213.0f,185.0f,255.0f,1.0f}; purple
    };
    FirstApp::FirstApp() { 
        globalPool = VeDescriptorPool::Builder(veDevice)
            .setMaxSets(VeSwapChain::MAX_FRAMES_IN_FLIGHT * (1 +  10))  
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VeSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VeSwapChain::MAX_FRAMES_IN_FLIGHT * 5)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VeSwapChain::MAX_FRAMES_IN_FLIGHT * 5)
            #ifdef MACOS
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            #endif
            .build();
        loadGameObjects(); 
        loadTextures();

        VkDescriptorPoolSize pool_sizes[] = {
            {VK_DESCRIPTOR_TYPE_SAMPLER,                1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000}
        };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t) IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        
        if(vkCreateDescriptorPool(veDevice.device(),
            &pool_info, nullptr, &imGuiPool) != VK_SUCCESS){
            throw std::runtime_error("Failed to create ImGui descriptor pool");
        }
    }
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
            .build();

        //create global descriptor pool
        std::vector<VkDescriptorSet> globalDescriptorSets(VeSwapChain::MAX_FRAMES_IN_FLIGHT);
        for(int i = 0; i < globalDescriptorSets.size(); i++){
            auto bufferInfo = uniformBuffers[i]->descriptorInfo();
            VeDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                .writeImage(1, textureInfos.data(),5)
                .writeImage(2, normalMapInfos.data(),5)
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

        //imgui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        //Dear IMGUI style
        // ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();
        ImGui::StyleColorsClassic();
    
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

        //setup renderpass for imgui
        createRenderPass();
        //setup renderer backend for imgui
        ImGui_ImplGlfw_InitForVulkan(veWindow.getGLFWWindow(), true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = veDevice.getInstance();
        init_info.PhysicalDevice = veDevice.getPhysicalDevice();
        init_info.Device = veDevice.device();
        init_info.QueueFamily = veDevice.graphicsQueueFamilyIndex();
        init_info.Queue = veDevice.graphicsQueue();
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = imGuiPool;
        init_info.Allocator = nullptr;
        init_info.MinImageCount = VeSwapChain::MAX_FRAMES_IN_FLIGHT;
        init_info.ImageCount = VeSwapChain::MAX_FRAMES_IN_FLIGHT;
        init_info.RenderPass = renderPass;
        init_info.Subpass = 0;

        ImGui_ImplVulkan_Init(&init_info);
        bool showPropertiesWindow = true;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
    
        // Set the window position to the right side
        ImGui::SetNextWindowPos(
            ImVec2(viewport->WorkSize.x - 200,  // X position (subtracting window width)
                viewport->WorkPos.y),        // Y position at the top
            ImGuiCond_Always
        );
        
        // Set the window size (optional)
        ImGui::SetNextWindowSize(
            ImVec2(200,                         // Width
                viewport->WorkSize.y),       // Height of the entire viewport
            ImGuiCond_Always
        );
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
                //start new imgui frame
                ImGui_ImplVulkan_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();
                sceneEditor.drawSceneEditor(gameObjects, lightPosition);

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
                ImGui::Render();
                ImDrawData *main_draw_data = ImGui::GetDrawData();
                if (main_draw_data) {
                    std::cout << "Draw data exists with " << main_draw_data->CmdListsCount << " command lists\n";
                    ImGui_ImplVulkan_RenderDrawData(main_draw_data, commandBuffer);
                }
                veRenderer.endSwapChainRenderPass(commandBuffer);
                veRenderer.endFrame();
            }
        }
        vkDeviceWaitIdle(veDevice.device()); //cpu wait for gpu to finish
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
    
    void FirstApp::loadGameObjects() {
        //object 1: cube
        std::shared_ptr<VeModel> veModel = VeModel::createModelFromFile(veDevice, "models/smooth_vase.obj");
        veModel->setTextureIndex(1);
        veModel->setNormalIndex(1);
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
        auto quad = VeGameObject::createGameObject();
        quad.model = veModel;
        quad.transform.translation = {0.0f, 0.5f, 0.0f};
        quad.transform.scale = {3.0f, 0.5f, 3.0f};
        quad.color = {103.0f,242.0f,209.0f};//light green
        quad.setTitle("Floor");
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
            textureInfos.push_back(VkDescriptorImageInfo(imageInfo));
            VkDescriptorImageInfo normalImageInfo{};
            normalImageInfo.imageLayout = normalMaps[i]->getLayout();
            normalImageInfo.imageView = normalMaps[i]->getNormalImageView();
            normalImageInfo.sampler = normalMaps[i]->getNormalSampler();
            normalMapInfos.push_back(VkDescriptorImageInfo(normalImageInfo)); 
        }
    }
    void FirstApp::createRenderPass() {
        vkDestroyRenderPass(veDevice.device(), renderPass, nullptr);

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = veRenderer.getSwapChainDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = veRenderer.getSwapChainImageFormat();
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency = {};

        dependency.dstSubpass = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.srcAccessMask = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(veDevice.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

}