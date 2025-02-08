#pragma once
#include "ve_window.hpp"
#include "ve_device.hpp"
#include "ve_game_object.hpp"
#include "ve_renderer.hpp"
#include "ve_descriptors.hpp"
#include "ve_texture.hpp"
#include "ve_normal_map.hpp"
#include "scene_editor_gui.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
namespace ve {
    class FirstApp{
        public:
            static constexpr int WIDTH = 1280;
            static constexpr int HEIGHT = 720;
            FirstApp();
            ~FirstApp();
            FirstApp(const FirstApp&) = delete;
            FirstApp& operator=(const FirstApp&) = delete;
            void run();
             
        private:
            void loadGameObjects();
            void loadTextures();
            int getNumLights();
            VeWindow veWindow{WIDTH, HEIGHT, "First App"};
            VeDevice veDevice{veWindow};
            VeRenderer veRenderer{veWindow, veDevice};
            VkRenderPass renderPass = VK_NULL_HANDLE;
            VkDescriptorPool imGuiPool;
            std::unique_ptr<VeDescriptorPool> globalPool{};
            std::vector<std::unique_ptr<VeTexture>> textures;
            std::vector<std::unique_ptr<VeNormal>> normalMaps;
            std::vector<std::unique_ptr<VeNormal>> specularMaps;
            std::vector<VkDescriptorImageInfo> textureInfos;
            std::vector<VkDescriptorImageInfo> normalMapInfos;
            std::vector<VkDescriptorImageInfo> specularMapInfos;
            SceneEditor sceneEditor{};
            VeGameObject::Map gameObjects;
            int selectedObject = -1;
    };
}