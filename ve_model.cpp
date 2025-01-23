#include "ve_model.hpp"
#include "utility.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <cassert>
#include <cstring>
#include <unordered_map>
#include <iostream>

namespace std{
    template<> struct hash<ve::VeModel::Vertex>{
        size_t operator()(const ve::VeModel::Vertex& vertex) const{
            size_t seed = 0.0f;
            ve::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
            return seed;
        }
    };
}
namespace ve{
    VeModel::VeModel(VeDevice& device, const VeModel::Builder &builder): veDevice(device){
        createVertexBuffers(builder.vertices);
        createIndexBuffers(builder.indices);
    }
    //buffer cleanup handled by Buffer class
    VeModel::~VeModel(){}
    void VeModel::createVertexBuffers(const std::vector<Vertex>& vertices){
        vertexCount = static_cast<uint32_t>(vertices.size());
        assert(vertexCount >= 3 && "Vertex count must be at least 3");
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        //create staging buffer
        uint32_t vertexSize = sizeof(vertices[0]);
        VeBuffer stagingBuffer{veDevice, vertexSize, vertexCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
        stagingBuffer.map();
        stagingBuffer.writeToBuffer((void *)vertices.data());

        vertexBuffer = std::make_unique<VeBuffer>(veDevice, vertexSize, vertexCount, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        veDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
    }
    void VeModel::createIndexBuffers(const std::vector<uint32_t>& indices){
        indexCount = static_cast<uint32_t>(indices.size());
        hasIndexBuffer =  indexCount > 0;
        //index buffer is optional
        if(!hasIndexBuffer){
            return;
        }
        VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
        uint32_t indexSize = sizeof(indices[0]);
        //create staging buffer
        VeBuffer stagingBuffer{veDevice, indexSize, indexCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
        stagingBuffer.map();
        stagingBuffer.writeToBuffer((void *)indices.data());
        //create index buffer
        indexBuffer = std::make_unique<VeBuffer>(veDevice, indexSize, indexCount, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        veDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
    }

    void VeModel::bind(VkCommandBuffer commandBuffer){
        VkBuffer buffers[] = {vertexBuffer->getBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
        if(hasIndexBuffer){
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
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
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
        attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
        attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
        attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
        attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});
        attributeDescriptions.push_back({4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)});
        return attributeDescriptions;
    }
    
    std::unique_ptr<VeModel> VeModel::createModelFromFile(VeDevice& device, const std::string& filePath){
        Builder builder{};
        builder.loadModel(filePath);
        return std::make_unique<VeModel>(device, builder);
    }
    void VeModel::Builder::loadModel(const std::string& filePath){
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn;
        std::string err;

        if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str())){
            throw std::runtime_error(warn + err);
        }
        vertices.clear();
        indices.clear();
        std::unordered_map<Vertex, uint32_t> uniqueVertices{};
        for(const auto& shape: shapes){
            for(const auto& index: shape.mesh.indices){
                Vertex vertex{};
                if(index.vertex_index >= 0){
                    vertex.position = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]
                    };
                }
                vertex.color = {
                    attrib.colors[3 * index.vertex_index + 0],
                    attrib.colors[3 * index.vertex_index + 1],
                    attrib.colors[3 * index.vertex_index + 2]
                };
                if(index.normal_index >= 0){
                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                    };
                }
                if(index.texcoord_index >= 0){
                    vertex.uv = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1],
                    };     
                }
    
                //if vertex is unique, add to list
                if(uniqueVertices.count(vertex) == 0){
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(uniqueVertices[vertex]);
            }
            //compute tangents for each triangle (access 3 vertices at each loop step)
            for (long index = 0;index < indices.size(); index+=3){
                //edges: edge1 = vertex2 - vertex1, edge2 = vertex3 - vertex1
                glm::vec3 edge1 = vertices[indices[index + 1]].position - vertices[indices[index]].position;
                glm::vec3 edge2 = vertices[indices[index + 2]].position - vertices[indices[index]].position;
                //delta uv: deltaUV1 = uv2 - uv1, deltaUV2 = uv3 - uv1
                glm::vec2 deltaUV1 = vertices[indices[index + 1]].uv - vertices[indices[index]].uv;
                glm::vec2 deltaUV2 = vertices[indices[index + 2]].uv - vertices[indices[index]].uv;
                //compute tangent
                float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
                glm::vec3 tangent;
                tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
                //store tangent for each triangle
                vertices[indices[index]].tangent = tangent;
                vertices[indices[index + 1]].tangent = tangent;
                vertices[indices[index + 2]].tangent = tangent;

            }
        }
    }


}