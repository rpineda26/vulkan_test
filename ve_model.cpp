#include "ve_model.hpp"
#include <cassert>
namespace ve{
    VeModel::VeModel(VeDevice& device, const std::vector<Vertex>& vertices): veDevice(device){
        createVertexBuffers(vertices);
    }
    VeModel::~VeModel(){
        vkDestroyBuffer(veDevice.device(), vertexBuffer, nullptr);
        vkFreeMemory(veDevice.device(), vertexBufferMemory, nullptr);
    }
    void VeModel::createVertexBuffers(const std::vector<Vertex>& vertices){
        vertexCount = static_cast<uint32_t>(vertices.size());
        assert(vertexCount >= 3 && "Vertex count must be at least 3");
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        veDevice.createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer, vertexBufferMemory);
        void* data;
        vkMapMemory(veDevice.device(), vertexBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(veDevice.device(), vertexBufferMemory);
    }

    void VeModel::bind(VkCommandBuffer commandBuffer){
        VkBuffer buffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
    }
    void VeModel::draw(VkCommandBuffer commandBuffer){
        vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
    }
    std::vector<VkVertexInputBindingDescription> VeModel::Vertex::getBindingDescriptions(){
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescriptions;
    }
    std::vector<VkVertexInputAttributeDescription> VeModel::Vertex::getAttributeDescriptions(){
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
        //vert
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; //RG
        attributeDescriptions[0].offset = offsetof(Vertex, position);
        //frag
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; //RGB
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        return attributeDescriptions;
    }
    
}