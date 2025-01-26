#pragma once
#include "ve_window.hpp"
#include "ve_device.hpp"
#include "ve_game_object.hpp"
#include "ve_renderer.hpp"
#include "ve_descriptors.hpp"
#include "ve_texture.hpp"
#include "ve_normal_map.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
namespace ve {
    class FirstApp{
        public:
            static constexpr int WIDTH = 800;
            static constexpr int HEIGHT = 600;
            FirstApp();
            ~FirstApp();
            FirstApp(const FirstApp&) = delete;
            FirstApp& operator=(const FirstApp&) = delete;
            void run();
             
        private:
            void loadGameObjects();
            void loadTextures();
            VeWindow veWindow{WIDTH, HEIGHT, "First App"};
            VeDevice veDevice{veWindow};
            VeRenderer veRenderer{veWindow, veDevice};

            std::unique_ptr<VeDescriptorPool> globalPool{};
            std::vector<std::unique_ptr<VeTexture>> textures;
            std::vector<std::unique_ptr<VeNormal>> normalMaps;
            std::vector<std::unique_ptr<VkDescriptorImageInfo>> textureInfos;
            std::vector<std::unique_ptr<VkDescriptorImageInfo>> normalMapInfos;

            VeGameObject::Map gameObjects;
    };
}