#pragma once
#include "ve_device.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <memory>

namespace ve{
    class VeModel{
    public:
        struct Vertex{
            glm::vec3 position;
            glm::vec3 color;
            glm::vec3 normal;
            glm::vec2 uv;
            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
        };
        struct Builder{
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            void loadModel(const std::string& filePath);
        };

        VeModel(VeDevice& device, const VeModel::Builder& builder);
        ~VeModel();
        VeModel(const VeModel&) = delete;
        VeModel& operator=(const VeModel&) = delete;

        static std::unique_ptr<VeModel> createModelFromFile(VeDevice& device, const std::string& filePath);
        void bind(VkCommandBuffer commandBuffer);
        void draw(VkCommandBuffer commandBuffer);

    private:
        void createVertexBuffers(const std::vector<Vertex>& vertices);
        void createIndexBuffers(const std::vector<uint32_t>& indices);  
        //attributes
        VeDevice& veDevice;
        //vertex buffer
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        uint32_t vertexCount;
        //index buffer
        bool hasIndexBuffer{false};
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;
        uint32_t indexCount;
    };
}