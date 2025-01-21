#pragma once

#include "ve_model.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <unordered_map>
namespace ve{
    struct TransformComponent{
        glm::vec3 translation{};
        glm::vec3 scale{1.0f,1.0f,1.0f};
        glm::vec3 rotation{};  
        glm::mat4 mat4();
        glm::mat3 normalMatrix();
    };
    class VeGameObject { 
        public:
            using id_t = unsigned int;
            using Map = std::unordered_map<id_t, VeGameObject>;
            static VeGameObject createGameObject(){ 
                static id_t currentId = 0; 
                return VeGameObject{currentId++}; 
            }
            VeGameObject(const VeGameObject&) = delete;
            VeGameObject& operator=(const VeGameObject&) = delete;
            VeGameObject(VeGameObject&&) = default;
            VeGameObject& operator=(VeGameObject&&) = default;
            id_t getId() { return id; }
            std::shared_ptr<VeModel> model{};
            glm::vec3  color{};
            TransformComponent transform{};
        private:
            VeGameObject(id_t objId): id{objId} {}
            id_t id;
    };

}