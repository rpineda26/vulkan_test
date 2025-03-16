#include "ve_imgui.hpp"
#include <iostream>
namespace ve{
    VkDescriptorPool VeImGui::createDescriptorPool(VkDevice device){
        VkDescriptorPool imGuiPool;
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
        
        if(vkCreateDescriptorPool(device,
            &pool_info, nullptr, &imGuiPool) != VK_SUCCESS){
            throw std::runtime_error("Failed to create ImGui descriptor pool");
        }
        return imGuiPool;
    }
    VkRenderPass VeImGui::createRenderPass(VkDevice device, VkFormat imageFormat, VkFormat depthFormat){
        
        VkRenderPass renderPass = VK_NULL_HANDLE;
        vkDestroyRenderPass(device, renderPass, nullptr);

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthFormat;
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
        colorAttachment.format = imageFormat;
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

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
        return renderPass;
    }

    void VeImGui::createImGuiContext( VeDevice& veDevice, VeWindow& veWindow, VkDescriptorPool imGuiPool, VkRenderPass renderPass, int imageCount){
        //imgui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        
        //Dear IMGUI style
        // ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();
        ImGui::StyleColorsClassic();
    
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

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
        init_info.MinImageCount = imageCount;
        init_info.ImageCount = imageCount;
        init_info.RenderPass = renderPass;
        init_info.Subpass = 0;

        ImGui_ImplVulkan_Init(&init_info);
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
    }
    void VeImGui::initializeImGuiFrame(){
         ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }
    void VeImGui::renderImGuiFrame(VkCommandBuffer commandBuffer){
        ImGui::Render();
        ImDrawData *main_draw_data = ImGui::GetDrawData();
        if (main_draw_data) {
            ImGui_ImplVulkan_RenderDrawData(main_draw_data, commandBuffer);
        }
    }
    void VeImGui::cleanUpImGui(){
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}