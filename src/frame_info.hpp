#pragma once

#include "ve_camera.hpp"
#include "ve_game_object.hpp"
#include <vulkan/vulkan.h>

#define MAX_POINT_LIGHTS 10
namespace ve{
    struct alignas(16)PointLight{
        glm::vec4 position{}; //ignore w, align with 16 bytes
        glm::vec4 color{}; // w is intensity
        float radius;
        // Color{1.0f, 0.9f, 0.5f, 1.0f}; yellowish
        // Color{213.0f,185.0f,255.0f,1.0f}; purple
    };
    struct alignas(16) GlobalUbo {
        glm::mat4 projection{1.0f};
        glm::mat4 view{1.0f};
        glm::mat4 inverseView{1.0f};
        glm::vec4 ambientLightColor{1.0f,1.0f,1.0f,0.1f};
        PointLight pointLights[MAX_POINT_LIGHTS];
        int numLights;
    };

    struct FrameInfo{
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        VeCamera& camera;
        VkDescriptorSet descriptorSet;
        VeGameObject::Map& gameObjects;
    };
}