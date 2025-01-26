#pragma once

#include "ve_camera.hpp"
#include "ve_game_object.hpp"
#include <vulkan/vulkan.h>

namespace ve{
    typedef enum SelectedObject{
        CAMERA = 1,
        LIGHT = 2,
        VASE = 3,
        CUBE = 4,
        FLOOR = 5
    } SelectedObject;
    struct FrameInfo{
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        VeCamera& camera;
        VkDescriptorSet descriptorSet;
        VeGameObject::Map& gameObjects;
    };
}