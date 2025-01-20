#include "ve_model.hpp"
#include <cassert>
namespace ve{
    VeModel::VeModel(VeDevice& device, const VeModel::Builder &builder): veDevice(device){
        createVertexBuffers(builder.vertices);
        createIndexBuffers(builder.indices);
    }
    VeModel::~VeModel(){
        vkDestroyBuffer(veDevice.device(), vertexBuffer, nullptr);
        vkFreeMemory(veDevice.device(), vertexBufferMemory, nullptr);
        if(hasIndexBuffer){
            vkDestroyBuffer(veDevice.device(), indexBuffer, nullptr);
            vkFreeMemory(veDevice.device(), indexBufferMemory, nullptr);
        }
    }
    void VeModel::createVertexBuffers(const std::vector<Vertex>& vertices){
        vertexCount = static_cast<uint32_t>(vertices.size());
        assert(vertexCount >= 3 && "Vertex count must be at least 3");
        //create staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        veDevice.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
        void* data;
        vkMapMemory(veDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(veDevice.device(), stagingBufferMemory);

        veDevice.createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
        veDevice.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
        vkDestroyBuffer(veDevice.device(), stagingBuffer, nullptr);
        vkFreeMemory(veDevice.device(), stagingBufferMemory, nullptr);
    }
    void VeModel::createIndexBuffers(const std::vector<uint32_t>& indices){
        indexCount = static_cast<uint32_t>(indices.size());
        hasIndexBuffer =  indexCount > 0;
        //index buffer is optional
        if(!hasIndexBuffer){
            return;
        }
        VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
        //create staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        veDevice.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
        void* data;
        vkMapMemory(veDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(veDevice.device(),stagingBufferMemory);

        veDevice.createBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
        veDevice.copyBuffer(stagingBuffer, indexBuffer, bufferSize);
        vkDestroyBuffer(veDevice.device(), stagingBuffer, nullptr);
        vkFreeMemory(veDevice.device(), stagingBufferMemory, nullptr);
    }
    void VeModel::bind(VkCommandBuffer commandBuffer){
        VkBuffer buffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
        if(hasIndexBuffer){
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }
    }
    void VeModel::draw(VkCommandBuffer commandBuffer){
        if(hasIndexBuffer){
            vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
        }else{
            vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
        }
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
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; //RG
        attributeDescriptions[0].offset = offsetof(Vertex, position);
        //frag
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; //RGB
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        return attributeDescriptions;
    }
    
}