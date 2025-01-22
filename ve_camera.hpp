#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace ve{
    class VeCamera{
        public:
            VeCamera() = default;
            //methods
            void setOrtho(float left, float right, float bottom, float top, float near, float far);
            void setPerspective(float fovy, float aspect, float near, float far);
            void setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3{0.0f, -1.0f, 0.0f});
            void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{0.0f, -1.0f, 0.0f});
            void setViewYXZ(glm::vec3 position, glm::vec3 rotation);
            //getters
            const glm::mat4& getProjectionMatrix() const { return projectionMatrix; }
            const glm::mat4& getViewMatrix() const { return viewMatrix; }
            const glm::mat4& getInverseMatrix() const { return inverseMatrix; }
        private:
            glm::mat4 projectionMatrix{1.0f};
            glm::mat4 viewMatrix{1.0f};
            glm::mat4 inverseMatrix{1.0f};
    };
}