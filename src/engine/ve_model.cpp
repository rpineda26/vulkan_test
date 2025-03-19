#include "ve_model.hpp"
#include  "buffer.hpp"
#include "utility.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
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
    void VeModel::updateAnimation(float deltaTime, int frameCounter){
        if(hasAnimation){
            // std::cout << "Animation updated" << std::endl;
            animationManager->update(deltaTime, *skeleton, frameCounter);
            skeleton->update();
            //update buffer
            shaderJointsBuffer->writeToBuffer(skeleton->jointMatrices.data());
            shaderJointsBuffer->flush();
            
            // for (size_t i = 0; i < 1; ++i) {
            //     std::cout << "Matrix " << i << ":" << std::endl;
            //     for (int row = 0; row < 4; row++) {
            //         for (int col = 0; col < 4; col++) {
            //             std::cout << skeleton->jointMatrices[6][col][row] << " ";
            //         }
            //         std::cout << std::endl;
            //     }
            //     std::cout << std::endl;
            // }      
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
        attributeDescriptions.push_back({5, 0, VK_FORMAT_R32G32B32A32_SINT, offsetof(Vertex, jointIndices)});
        attributeDescriptions.push_back({6, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, jointWeights)});
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
        
        auto model = std::make_unique<VeModel>(device, builder);
        if(extension == "gltf" || extension == "glb"){
            model->loadSkeleton(builder.model);
            model->loadAnimations(builder.model);
        }
        return model;
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
                vertex.jointIndices = glm::ivec4(0);
                vertex.jointWeights = glm::vec4(1.0f,0.0f,0.0f,0.0f);
    
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
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;
        std::string fullPath = std::string(ENGINE_DIR) + filePath;
        bool ret = false;
        std::cout << "Loading model: " << fullPath << std::endl;
        
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
            std::cerr<<"Failed to load gltf file";
        }
        // //load buffers
        // for(const auto& buffer: model.buffers){
        //     std::cout << "Buffer: " << buffer.name << std::endl;
        // }
        // //load images
        // for(const auto& image: model.images){
        //     std::cout << "Image: " << image.name << std::endl;
        // }
        // //load materials
        // for(const auto& material: model.materials){
        //     std::cout << "Material: " << material.name << std::endl;
        // }
        // //load meshes
        // for(const auto& mesh: model.meshes){
        //     std::cout << "Mesh: " << mesh.name << std::endl;
        // }
        // //load nodes
        // for(const auto& node: model.nodes){
        //     std::cout << "Node: " << node.name << std::endl;
        // }
        // //load textures
        // for(const auto& texture: model.textures){
        //     std::cout << "Texture: " << texture.name << std::endl;
        // }

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
                const auto& jointsAccessorIt = primitive.attributes.find("JOINTS_0");
                const auto& weightsAccessorIt = primitive.attributes.find("WEIGHTS_0");
                
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
                
                 // New - get joint index data if available
                const tinygltf::Accessor* jointsAccessor = nullptr;
                const tinygltf::BufferView* jointsBufferView = nullptr;
                const tinygltf::Buffer* jointsBuffer = nullptr;
                if (jointsAccessorIt != primitive.attributes.end()) {
                    jointsAccessor = &model.accessors[jointsAccessorIt->second];
                    jointsBufferView = &model.bufferViews[jointsAccessor->bufferView];
                    jointsBuffer = &model.buffers[jointsBufferView->buffer];
                }
                
                // New - get joint weight data if available
                const tinygltf::Accessor* weightsAccessor = nullptr;
                const tinygltf::BufferView* weightsBufferView = nullptr;
                const tinygltf::Buffer* weightsBuffer = nullptr;
                if (weightsAccessorIt != primitive.attributes.end()) {
                    weightsAccessor = &model.accessors[weightsAccessorIt->second];
                    weightsBufferView = &model.bufferViews[weightsAccessor->bufferView];
                    weightsBuffer = &model.buffers[weightsBufferView->buffer];
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
                // New - Load joint indices if available
                if (jointsAccessor) {
                    // Joint indices are usually stored as vec4 of unsigned bytes or unsigned shorts
                    if (jointsAccessor->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                        const uint8_t* jointsData = reinterpret_cast<const uint8_t*>(
                            &jointsBuffer->data[jointsBufferView->byteOffset + jointsAccessor->byteOffset]);
                        for (size_t i = 0; i < vertexCount; i++) {
                            tempVertices[i].jointIndices = {
                                static_cast<int>(jointsData[i * 4 + 0]),
                                static_cast<int>(jointsData[i * 4 + 1]),
                                static_cast<int>(jointsData[i * 4 + 2]),
                                static_cast<int>(jointsData[i * 4 + 3])
                            };
                            // Add debugging print for joint indices
                            if (i % 100 == 0) { // Print every 100th vertex to avoid flooding console
                                std::cout << "Vertex " << i << " Joint Indices: " 
                                        << tempVertices[i].jointIndices.x << " "
                                        << tempVertices[i].jointIndices.y << " "
                                        << tempVertices[i].jointIndices.z << " "
                                        << tempVertices[i].jointIndices.w << std::endl;
                            }
                        }
                    } else if (jointsAccessor->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                        const uint16_t* jointsData = reinterpret_cast<const uint16_t*>(
                            &jointsBuffer->data[jointsBufferView->byteOffset + jointsAccessor->byteOffset]);
                        for (size_t i = 0; i < vertexCount; i++) {
                            tempVertices[i].jointIndices = {
                                static_cast<int>(jointsData[i * 4 + 0]),
                                static_cast<int>(jointsData[i * 4 + 1]),
                                static_cast<int>(jointsData[i * 4 + 2]),
                                static_cast<int>(jointsData[i * 4 + 3])
                            };
                            // Add debugging print for joint indices
                            if (i % 100 == 0) { // Print every 100th vertex to avoid flooding console
                                std::cout << "Vertex " << i << " Joint Indices: " 
                                        << tempVertices[i].jointIndices.x << " "
                                        << tempVertices[i].jointIndices.y << " "
                                        << tempVertices[i].jointIndices.z << " "
                                        << tempVertices[i].jointIndices.w << std::endl;
                            }
                        }
                    }
                }
            
                // New - Load joint weights if available
                if (weightsAccessor) {
                    const float* weightsData = reinterpret_cast<const float*>(
                        &weightsBuffer->data[weightsBufferView->byteOffset + weightsAccessor->byteOffset]);
                    for (size_t i = 0; i < vertexCount; i++) {
                        tempVertices[i].jointWeights = {
                            weightsData[i * 4 + 0],
                            weightsData[i * 4 + 1],
                            weightsData[i * 4 + 2],
                            weightsData[i * 4 + 3]
                        };
                        
                        // Normalize weights to ensure they sum to 1.0
                        float sum = tempVertices[i].jointWeights.x + 
                                    tempVertices[i].jointWeights.y + 
                                    tempVertices[i].jointWeights.z + 
                                    tempVertices[i].jointWeights.w;
                        
                        if (sum > 0.0f) {
                            tempVertices[i].jointWeights /= sum;
                        } else {
                            // If no weights, assign fully to the first joint
                            tempVertices[i].jointWeights = { 1.0f, 0.0f, 0.0f, 0.0f };
                        }
                        if(i % 100 == 0) { // Print every 100th vertex to avoid flooding console
                            std::cout << "Joint Weights: " << tempVertices[i].jointWeights.x << " " << tempVertices[i].jointWeights.y << " " << tempVertices[i].jointWeights.z << " " << tempVertices[i].jointWeights.w << std::endl;
                        }
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
    void VeModel::loadSkeleton(const tinygltf::Model& model){
        size_t numSkeletons = model.skins.size();
        if(!numSkeletons)
            return;

        skeleton = std::make_unique<Skeleton>();
        
        tinygltf::Skin skin = model.skins[0];
        if(skin.inverseBindMatrices!=-1){
            auto& joints = skeleton -> joints; 
            size_t numJoints = skin.joints.size();
            joints.resize(numJoints);
            skeleton->name=skin.name;
            skeleton->jointMatrices.resize(numJoints);
            // std::cout << "Skeleton: " << skin.name << std::endl;
            
            const tinygltf::Accessor& invAccessor = model.accessors[skin.inverseBindMatrices];
            const tinygltf::BufferView& invBufferView = model.bufferViews[invAccessor.bufferView];
            const tinygltf::Buffer& invBuffer = model.buffers[invBufferView.buffer];
            
            // Debug info about inverse bind matrices accessor
            std::cout << "Inverse bind matrices accessor info:" << std::endl;
            std::cout << "  Component type: " << invAccessor.componentType << 
                        " (Should be " << TINYGLTF_COMPONENT_TYPE_FLOAT << " for float)" << std::endl;
            std::cout << "  Type: " << invAccessor.type << 
                        " (Should be " << TINYGLTF_TYPE_MAT4 << " for mat4)" << std::endl;
            std::cout << "  Count: " << invAccessor.count << std::endl;
            std::cout << "  Byte offset: " << invAccessor.byteOffset << std::endl;

            // Validate component type
            if (invAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
                throw std::runtime_error("Inverse bind matrices are not of type float.");
            }

            // Get direct pointer to float data - no need for transposition
            const float* inverseBindMatricesData = reinterpret_cast<const float*>(
                &invBuffer.data[invBufferView.byteOffset + invAccessor.byteOffset]);
            
                // Debug: Print buffer info
            std::cout << "Inverse bind matrices buffer info:" << std::endl;
            std::cout << "  Buffer size: " << invBuffer.data.size() << " bytes" << std::endl;
            std::cout << "  BufferView byteOffset: " << invBufferView.byteOffset << std::endl;
            std::cout << "  BufferView byteLength: " << invBufferView.byteLength << std::endl;
            std::cout << "  Total offset: " << (invBufferView.byteOffset + invAccessor.byteOffset) << std::endl;
            
            // Process each joint
            for (size_t i = 0; i < numJoints; i++) {
                int jointNodeIdx = skin.joints[i];
                
                // Setup joint data
                joints[i].name = model.nodes[jointNodeIdx].name;
                skeleton->nodeJointMap[jointNodeIdx] = i;
                //  inverse bind matrix
                joints[i].inverseBindMatrix = glm::make_mat4(&inverseBindMatricesData[i * 16]);
                 // local world magtrix
                extractNodeTransform(model.nodes[jointNodeIdx], joints[i]);
                joints[i].jointWorldMatrix = calculateLocalTransform(joints[i]);
                // Debug: Print inverse bind matrix for each joint
                std::cout << "Joint " << i << " (" << joints[i].name << ") - Inverse Bind Matrix:" << std::endl;
                for (int row = 0; row < 4; row++) {
                    std::cout << "  [";
                    for (int col = 0; col < 4; col++) {
                        std::cout << joints[i].inverseBindMatrix[col][row];
                        if (col < 3) std::cout << ", ";
                    }
                    std::cout << "]" << std::endl;
                }
                
                // Also print raw data values for verification
                std::cout << "  Raw data: ";
                for (int j = 0; j < 16; j++) {
                    std::cout << inverseBindMatricesData[i * 16 + j];
                    if (j < 15) std::cout << ", ";
                }
                std::cout << std::endl;
            }
            int rootJoint = skin.joints[0];
            loadJoints(rootJoint, -1, model);
            updateJointHierarchy(model);
        }
        //create shader buffer
        uint32_t numJoints = static_cast<uint32_t>(skeleton->joints.size());
        uint32_t jointSize = static_cast<uint32_t>(sizeof(glm::mat4));
        shaderJointsBuffer = std::make_unique<VeBuffer>(veDevice, numJoints * jointSize, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, veDevice.properties.limits.minUniformBufferOffsetAlignment);
        shaderJointsBuffer->map();
    }
    void VeModel::loadJoints(int nodeIndex, int parentIndex, const tinygltf::Model& model){
        int currentJoint = skeleton->nodeJointMap[nodeIndex];
        auto& joint = skeleton->joints[currentJoint];
        joint.parentIndex = parentIndex;
        size_t numChildren = model.nodes[nodeIndex].children.size();
        if(numChildren>0){
            joint.childrenIndices.resize(numChildren);
            for(size_t i = 0; i < numChildren; i++){
                int childNodeIndex = model.nodes[nodeIndex].children[i];
                joint.childrenIndices[i] = skeleton->nodeJointMap[childNodeIndex];
                loadJoints(childNodeIndex, currentJoint, model);
            }
        }
    }
    void VeModel::loadAnimations(const tinygltf::Model& model){
        if(!skeleton){
            std::cerr << "Error: Skeleton not loaded" << std::endl;
            return;
        }
        size_t numAnimations = model.animations.size();
        if(!numAnimations){
            std::cerr << "Error: No animations found" << std::endl;
            return;
        }
        animationManager = std::make_shared<AnimationManager>();
        for(size_t i = 0; i < numAnimations; i++){
            const tinygltf::Animation& animation = model.animations[i];
            std::string name = animation.name.empty() ? "animation" + std::to_string(i) : animation.name;
            std::shared_ptr<Animation> anim = std::make_shared<Animation>(name);
            //samplers
            size_t numSamplers = animation.samplers.size();
            anim->samplers.resize(numSamplers);
            for(size_t samplerIndex = 0; samplerIndex < numSamplers; samplerIndex++) {
                tinygltf::AnimationSampler gltfSampler = animation.samplers[samplerIndex];
                auto& sampler = anim->samplers[samplerIndex];
                sampler.interpolationMethod = Animation::InterpolationMethod::LINEAR;
                if(gltfSampler.interpolation == "STEP")
                    sampler.interpolationMethod = Animation::InterpolationMethod::STEP;
                
                // Process input timestamps
                const tinygltf::Accessor& inputAccessor = model.accessors[gltfSampler.input];
                const tinygltf::BufferView& inputBufferView = model.bufferViews[inputAccessor.bufferView];
                const tinygltf::Buffer& inputBuffer = model.buffers[inputBufferView.buffer];
                
                size_t count = inputAccessor.count;
                sampler.timeStamps.resize(count);
                
                // Get byte stride
                size_t inputByteStride = inputBufferView.byteStride;
                if (inputByteStride == 0) {
                    // If byteStride is not defined, calculate it based on the accessor's type and component type
                    inputByteStride = tinygltf::GetComponentSizeInBytes(inputAccessor.componentType) * 
                                    tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_SCALAR);
                }
                
                // Get buffer data pointer
                const unsigned char* inputBufferData = &inputBuffer.data[inputBufferView.byteOffset + 
                                                    inputAccessor.byteOffset];
                
                if (inputAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                    for (size_t i = 0; i < count; i++) {
                        const float* value = reinterpret_cast<const float*>(inputBufferData + i * inputByteStride);
                        sampler.timeStamps[i] = *value;
                    }
                } else {
                    std::cout << "Error: Animation sampler input component type not supported" << std::endl;
                }
                
                // Process output values
                const tinygltf::Accessor& outputAccessor = model.accessors[gltfSampler.output];
                const tinygltf::BufferView& outputBufferView = model.bufferViews[outputAccessor.bufferView];
                const tinygltf::Buffer& outputBuffer = model.buffers[outputBufferView.buffer];
                
                count = outputAccessor.count;
                sampler.TRSoutputValues.resize(count);
                    
                // Get byte stride
                size_t outputByteStride = outputBufferView.byteStride;
                if (outputByteStride == 0) {
                    // If byteStride is not defined, calculate it based on the accessor's type and component type
                    outputByteStride = tinygltf::GetComponentSizeInBytes(outputAccessor.componentType) * 
                                    tinygltf::GetNumComponentsInType(outputAccessor.type);
                }
                
                // Get buffer data pointer
                const unsigned char* outputBufferData = &outputBuffer.data[outputBufferView.byteOffset + 
                                                        outputAccessor.byteOffset];
                
                switch (outputAccessor.type) {
                    case TINYGLTF_TYPE_VEC3:
                    {
                        for (size_t i = 0; i < count; i++) {
                            const float* value = reinterpret_cast<const float*>(outputBufferData + i * outputByteStride);
                            sampler.TRSoutputValues[i] = glm::vec4(value[0], value[1], value[2], 0.0f);
                        }
                        break;
                    }
                    case TINYGLTF_TYPE_VEC4:
                    {
                        for (size_t i = 0; i < count; i++) {
                            const float* value = reinterpret_cast<const float*>(outputBufferData + i * outputByteStride);
                            sampler.TRSoutputValues[i] = glm::vec4(value[0], value[1], value[2], value[3]);
                        }
                        break;
                    }
                    default:
                    {
                        std::cout << "Error: Animation sampler output type not supported" << std::endl;
                        break;
                    }
                }
            }
            
      
            //atleast one samplers exist
            if (anim->samplers.size()){
                auto& sampler = anim->samplers[0];
                //interpolate between two or more time stamps
                if(sampler.timeStamps.size() >=2){
                    anim->setFirstKeyFrameTime(sampler.timeStamps[0]);
                    anim->setLastKeyFrameTime(sampler.timeStamps.back());
                }
            }
            //each node of the skeleton has channels that point to a sampler
            size_t numChannels = animation.channels.size();
            anim->channels.resize(numChannels);
            for(size_t channelIndex = 0; channelIndex < numChannels; channelIndex++){
                tinygltf::AnimationChannel gltfChannel = animation.channels[channelIndex];
                auto& channel = anim->channels[channelIndex];
                channel.node = skeleton->nodeJointMap[gltfChannel.target_node];
                channel.samplerIndex = gltfChannel.sampler;
                if(gltfChannel.target_path == "translation"){
                    channel.pathType = Animation::PathType::TRANSLATION;
                }else if(gltfChannel.target_path == "rotation"){
                    channel.pathType = Animation::PathType::ROTATION;
                }else if(gltfChannel.target_path == "scale"){
                    channel.pathType = Animation::PathType::SCALE;
                }else{
                    std::cerr << "Unknown channel target path: " << gltfChannel.target_path << std::endl;
                }
            }
            animationManager->push(anim);
            std::cout << "Animation loaded: " << anim->getName() << std::endl;
        }
        hasAnimation = (animationManager->size()) ? true : false;
    }
    // Extract translation, rotation, and scale from a node
    void VeModel::extractNodeTransform(const tinygltf::Node& node, Joint& joint) {
        // Default values
        joint.translation = glm::vec3(0.0f);
        joint.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
        joint.scale = glm::vec3(1.0f);

        // If matrix is directly provided
        if (!node.matrix.empty()) {
            glm::mat4 nodeMatrix = glm::make_mat4(node.matrix.data());
            
            // Decompose the matrix into TRS components
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(nodeMatrix, joint.scale, joint.rotation, joint.translation, skew, perspective);
        } else {
            // Extract individual TRS components if provided
            if (!node.translation.empty()) {
                joint.translation = glm::vec3(
                    node.translation[0], 
                    node.translation[1], 
                    node.translation[2]
                );
            }
        
            if (!node.rotation.empty()) {
                joint.rotation = glm::quat(
                    node.rotation[3], // w is last in glTF but first in glm
                    node.rotation[0], 
                    node.rotation[1], 
                    node.rotation[2]
                );
            }
            
            if (!node.scale.empty()) {
                joint.scale = glm::vec3(
                    node.scale[0], 
                    node.scale[1], 
                    node.scale[2]
                );
            }
        }
    }

    // Calculate local transform matrix from TRS components
    glm::mat4 VeModel::calculateLocalTransform(const Joint& joint) {
        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), joint.translation);
        glm::mat4 rotationMatrix = glm::mat4_cast(joint.rotation);
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), joint.scale);
        
        return translationMatrix * rotationMatrix * scaleMatrix;
    }

    // Update the joint hierarchy to calculate world matrices
    void VeModel::updateJointHierarchy(const tinygltf::Model& model) {
        // Find root joints (joints with parentIndex == -1)
        for (size_t i = 0; i < skeleton->joints.size(); i++) {
            if (skeleton->joints[i].parentIndex == -1) {
                // For root joints, local transform is the world transform
                skeleton->joints[i].jointWorldMatrix = calculateLocalTransform(skeleton->joints[i]);
                
                // Process children
                updateJointWorldMatrices(i);
            }
        }
    }

    // Recursively update joint world matrices
    void VeModel::updateJointWorldMatrices(int jointIndex) {
        Joint& joint = skeleton->joints[jointIndex];
        
        // Process all children
        for (int childIndex : joint.childrenIndices) {
            Joint& childJoint = skeleton->joints[childIndex];
            
            // Child's world matrix = parent's world matrix * child's local matrix
            glm::mat4 childLocalMatrix = calculateLocalTransform(childJoint);
            childJoint.jointWorldMatrix = joint.jointWorldMatrix * childLocalMatrix;
            
            // Recursively process this child's children
            updateJointWorldMatrices(childIndex);
        }
    }


}