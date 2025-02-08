#pragma once

#include "ve_model.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <unordered_map>
#include <cstring>
namespace ve{
    struct TransformComponent{
        glm::vec3 translation{};
        glm::vec3 scale{1.0f,1.0f,1.0f};
        glm::vec3 rotation{};  
        glm::mat4 mat4();
        glm::mat3 normalMatrix();
    };
    struct PointLightComponent{
        float lightIntensity = 1.0f;
    };
    class VeGameObject { 
        public:
            //user defined types
            using id_t = unsigned int;
            using Map = std::unordered_map<id_t, VeGameObject>;
            //instantiation of game Object
            static VeGameObject createGameObject(){ 
                static id_t currentId = 0; 
                return VeGameObject{currentId++}; 
            }
            //instantiation of point light
            static VeGameObject createPointLight(float intensity=1.0f, float radius=0.1f, glm::vec3 color=glm::vec3(1.0f));

            VeGameObject(const VeGameObject&) = delete;
            VeGameObject& operator=(const VeGameObject&) = delete;
            VeGameObject(VeGameObject&&) = default;
            VeGameObject& operator=(VeGameObject&&) = default;
            
            //attributes
            glm::vec3  color{1.0f};
            TransformComponent transform{};
            //optional attributes
            std::shared_ptr<VeModel> model{};
            std::unique_ptr<PointLightComponent> lightComponent = nullptr;

            //getters and setters
            id_t getId() { return id; }
            char* getTitle(){
                return title;
            }
            void setTitle(const char* newTitle){
                strncpy(title, newTitle, sizeof(title));
            }
            void setTextureIndex(uint32_t index){
                textureIndex = index;
            }
            void setNormalIndex(uint32_t index){
                normalIndex = index;
            }
            void setSpecularIndex(uint32_t index){
                specularIndex = index;
            }
            uint32_t getTextureIndex(){
                return textureIndex;
            }
            uint32_t getNormalIndex(){
                return normalIndex;
            }
            uint32_t getSpecularIndex(){
                return specularIndex;
            }
            void setSmoothness(float value){
                smoothness = value;
            }
            float getSmoothness(){
                return smoothness;
            }
        private:
            //instantiation of VeGameobject is only allowed through createGameObject to 
            //make sure id is unique (incrementing)
            VeGameObject(id_t objId): id{objId} {}
            id_t id;
            char title[26]; 
            uint32_t textureIndex = 0;
            uint32_t normalIndex = 0;
            uint32_t specularIndex = 0;
            float smoothness = 0.0f;
    };

}