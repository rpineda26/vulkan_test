#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace ve{
    class VeCamera{
        public:
            void setOrtho(float left, float right, float bottom, float top, float near, float far);
            void setPerspective(float fovy, float aspect, float near, float far);
            const glm::mat4& getProjectionMatrix() const { return projectionMatrix; }
        private:
            glm::mat4 projectionMatrix{1.0f};
    };
}