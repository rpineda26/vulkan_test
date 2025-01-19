#pragma once
#include "ve_device.hpp"

#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace ve{
    class VeModel{
    public:
        struct Vertex{
            glm::vec2 position;

            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
        };
        VeModel(VeDevice& device, const std::vector<Vertex>& vertices);
        ~VeModel();
        VeModel(const VeModel&) = delete;
        VeModel& operator=(const VeModel&) = delete;

        void bind(VkCommandBuffer commandBuffer);
        void draw(VkCommandBuffer commandBuffer);

    private:
        void createVertexBuffers(const std::vector<Vertex>& vertices);

        VeDevice& veDevice;
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        uint32_t vertexCount;
    };
}