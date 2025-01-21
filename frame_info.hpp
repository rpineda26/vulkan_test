#pragma once

#include "ve_camera.hpp"
#include "ve_game_object.hpp"
#include <vulkan/vulkan.h>

namespace ve{
    struct FrameInfo{
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        VeCamera& camera;
        VkDescriptorSet descriptorSet;
        VeGameObject::Map& gameObjects;
    };
}