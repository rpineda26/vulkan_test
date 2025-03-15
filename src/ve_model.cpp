#include "ve_model.hpp"
#include "utility.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <tiny_gltf.h>
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
    void VeModel::drawInstanced(VkCommandBuffer commandBuffer, uint32_t instanceCount){
        if(hasIndexBuffer){
            vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
        }else{
            vkCmdDraw(commandBuffer, vertexCount, instanceCount, 0, 0);
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
        std::string extension = filePath.substr(filePath.find_last_of(".") + 1);
        if (extension == "gltf" || extension == "glb") {
            builder.loadModelGLTF(filePath);
        } else {
            // Default to OBJ for other formats
            builder.loadModel(filePath);
        }
        
        return std::make_unique<VeModel>(device, builder);
    }
    void VeModel::Builder::loadModel(const std::string& filePath){
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn;
        std::string err;

        std::string fullPath = std::string(ENGINE_DIR) + filePath;
        if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fullPath.c_str())){
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
    void VeModel::Builder::loadModelGLTF(const std::string& filePath){
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;
        std::string fullPath = std::string(ENGINE_DIR) + filePath;
        bool ret = false;
        
        // Check if file is binary (.glb) or text (.gltf) format
        if (filePath.find(".glb") != std::string::npos) {
            ret = loader.LoadBinaryFromFile(&model, &err, &warn, fullPath);
        } else {
            ret = loader.LoadASCIIFromFile(&model, &err, &warn, fullPath);
        }
        
        if(!warn.empty()){
            std::cerr << "Warning: " << warn << std::endl;
        }
        if(!err.empty()){
            std::cerr << "Error: " << err << std::endl;
        }
        if(!ret){
            throw std::runtime_error("Failed to load gltf file");
        }
        //load buffers
        for(const auto& buffer: model.buffers){
            std::cout << "Buffer: " << buffer.name << std::endl;
        }
        //load images
        for(const auto& image: model.images){
            std::cout << "Image: " << image.name << std::endl;
        }
        //load materials
        for(const auto& material: model.materials){
            std::cout << "Material: " << material.name << std::endl;
        }
        //load meshes
        for(const auto& mesh: model.meshes){
            std::cout << "Mesh: " << mesh.name << std::endl;
        }
        //load nodes
        for(const auto& node: model.nodes){
            std::cout << "Node: " << node.name << std::endl;
        }
        //load textures
        for(const auto& texture: model.textures){
            std::cout << "Texture: " << texture.name << std::endl;
        }

        vertices.clear();
        indices.clear();
        std::unordered_map<Vertex, uint32_t> uniqueVertices{};
        for (const auto& mesh : model.meshes) {
            for (const auto& primitive : mesh.primitives) {
                // Get accessor indices for the attributes we need
                const auto& positionAccessorIt = primitive.attributes.find("POSITION");
                const auto& normalAccessorIt = primitive.attributes.find("NORMAL");
                const auto& texcoordAccessorIt = primitive.attributes.find("TEXCOORD_0");
                const auto& colorAccessorIt = primitive.attributes.find("COLOR_0");
                
                // Check if we have position data (required)
                if (positionAccessorIt == primitive.attributes.end()) {
                    continue;
                }
                
                // Get indices
                const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
                const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
                const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];
                
                // Get position data
                const tinygltf::Accessor& posAccessor = model.accessors[positionAccessorIt->second];
                const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
                const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];
                
                // Optional: Get normal data if available
                const tinygltf::Accessor* normAccessor = nullptr;
                const tinygltf::BufferView* normBufferView = nullptr;
                const tinygltf::Buffer* normBuffer = nullptr;
                if (normalAccessorIt != primitive.attributes.end()) {
                    normAccessor = &model.accessors[normalAccessorIt->second];
                    normBufferView = &model.bufferViews[normAccessor->bufferView];
                    normBuffer = &model.buffers[normBufferView->buffer];
                }
                
                // Optional: Get texcoord data if available
                const tinygltf::Accessor* texAccessor = nullptr;
                const tinygltf::BufferView* texBufferView = nullptr;
                const tinygltf::Buffer* texBuffer = nullptr;
                if (texcoordAccessorIt != primitive.attributes.end()) {
                    texAccessor = &model.accessors[texcoordAccessorIt->second];
                    texBufferView = &model.bufferViews[texAccessor->bufferView];
                    texBuffer = &model.buffers[texBufferView->buffer];
                }
                
                // Optional: Get color data if available
                const tinygltf::Accessor* colorAccessor = nullptr;
                const tinygltf::BufferView* colorBufferView = nullptr;
                const tinygltf::Buffer* colorBuffer = nullptr;
                if (colorAccessorIt != primitive.attributes.end()) {
                    colorAccessor = &model.accessors[colorAccessorIt->second];
                    colorBufferView = &model.bufferViews[colorAccessor->bufferView];
                    colorBuffer = &model.buffers[colorBufferView->buffer];
                }
                
                // Number of vertices
                size_t vertexCount = posAccessor.count;
                
                // Create temporary vertices
                std::vector<Vertex> tempVertices(vertexCount);
                
                // Load position data
                const float* posData = reinterpret_cast<const float*>(&posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]);
                for (size_t i = 0; i < vertexCount; i++) {
                    tempVertices[i].position = {
                        posData[i * 3 + 0],
                        posData[i * 3 + 1],
                        posData[i * 3 + 2]
                    };
                }
                
                // Load normal data if available
                if (normAccessor) {
                    const float* normData = reinterpret_cast<const float*>(&normBuffer->data[normBufferView->byteOffset + normAccessor->byteOffset]);
                    for (size_t i = 0; i < vertexCount; i++) {
                        tempVertices[i].normal = {
                            normData[i * 3 + 0],
                            normData[i * 3 + 1],
                            normData[i * 3 + 2]
                        };
                    }
                } else {
                    // Default normals
                    for (size_t i = 0; i < vertexCount; i++) {
                        tempVertices[i].normal = { 0.0f, 1.0f, 0.0f };
                    }
                }
                
                // Load texcoord data if available
                if (texAccessor) {
                    const float* texData = reinterpret_cast<const float*>(&texBuffer->data[texBufferView->byteOffset + texAccessor->byteOffset]);
                    for (size_t i = 0; i < vertexCount; i++) {
                        tempVertices[i].uv = {
                            texData[i * 2 + 0],
                            texData[i * 2 + 1]
                        };
                    }
                } else {
                    // Default UVs
                    for (size_t i = 0; i < vertexCount; i++) {
                        tempVertices[i].uv = { 0.0f, 0.0f };
                    }
                }
                
                // Load color data if available
                if (colorAccessor) {
                    const float* colorData = reinterpret_cast<const float*>(&colorBuffer->data[colorBufferView->byteOffset + colorAccessor->byteOffset]);
                    for (size_t i = 0; i < vertexCount; i++) {
                        if (colorAccessor->type == TINYGLTF_TYPE_VEC3) {
                            tempVertices[i].color = {
                                colorData[i * 3 + 0],
                                colorData[i * 3 + 1],
                                colorData[i * 3 + 2]
                            };
                        } else if (colorAccessor->type == TINYGLTF_TYPE_VEC4) {
                            // If color is vec4, just use the RGB components
                            tempVertices[i].color = {
                                colorData[i * 4 + 0],
                                colorData[i * 4 + 1],
                                colorData[i * 4 + 2]
                            };
                        }
                    }
                } else {
                    // Default colors
                    for (size_t i = 0; i < vertexCount; i++) {
                        tempVertices[i].color = { 1.0f, 1.0f, 1.0f };
                    }
                }
                
                // Load indices
                std::vector<uint32_t> tempIndices;
                const uint8_t* indexData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];
                
                // Handle different index component types
                if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    const uint16_t* shortIndices = reinterpret_cast<const uint16_t*>(indexData);
                    for (size_t i = 0; i < indexAccessor.count; i++) {
                        tempIndices.push_back(static_cast<uint32_t>(shortIndices[i]));
                    }
                } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    const uint32_t* intIndices = reinterpret_cast<const uint32_t*>(indexData);
                    for (size_t i = 0; i < indexAccessor.count; i++) {
                        tempIndices.push_back(intIndices[i]);
                    }
                } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                    const uint8_t* byteIndices = indexData;
                    for (size_t i = 0; i < indexAccessor.count; i++) {
                        tempIndices.push_back(static_cast<uint32_t>(byteIndices[i]));
                    }
                }
                
                // Add vertices and indices to the model
                for (size_t i = 0; i < tempIndices.size(); i++) {
                    Vertex vertex = tempVertices[tempIndices[i]];
                    
                    if (uniqueVertices.count(vertex) == 0) {
                        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                        vertices.push_back(vertex);
                    }
                    indices.push_back(uniqueVertices[vertex]);
                }
            }
        }
        
        // Compute tangents for normal mapping
        for (size_t i = 0; i < indices.size(); i += 3) {
            if (i + 2 >= indices.size()) break;
            
            glm::vec3 edge1 = vertices[indices[i + 1]].position - vertices[indices[i]].position;
            glm::vec3 edge2 = vertices[indices[i + 2]].position - vertices[indices[i]].position;
            
            glm::vec2 deltaUV1 = vertices[indices[i + 1]].uv - vertices[indices[i]].uv;
            glm::vec2 deltaUV2 = vertices[indices[i + 2]].uv - vertices[indices[i]].uv;
            
            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
            if (std::isfinite(f)) {
                glm::vec3 tangent;
                tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
                
                vertices[indices[i]].tangent = tangent;
                vertices[indices[i + 1]].tangent = tangent;
                vertices[indices[i + 2]].tangent = tangent;
            }
        }
    }


}