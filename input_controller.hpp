#pragma once

#include "ve_game_object.hpp"
#include "ve_window.hpp"

namespace ve{
    class InputController{
        public:
        struct KeyMappings{
            //movement
            int forward{GLFW_KEY_W};
            int backward{GLFW_KEY_S};
            int leftward{GLFW_KEY_A};
            int rightward{GLFW_KEY_D};
            int upward{GLFW_KEY_Q};
            int downward{GLFW_KEY_E};
            //tilt
            int tiltUp{GLFW_KEY_UP};
            int tiltDown{GLFW_KEY_DOWN};
            int tiltLeft{GLFW_KEY_LEFT};
            int tiltRight{GLFW_KEY_RIGHT};
            //zoom
            int zoomIn{GLFW_KEY_Z};
            int zoomOut{GLFW_KEY_X};
        };
            void moveInPlane(GLFWwindow* window, float deltaTime, VeGameObject& gameObject);
            //for testing lighting
            void movePosition(GLFWwindow* window, float deltaTime, glm::vec3& lightPosition);
            KeyMappings keyMappings{};
            float moveSpeed{1.0f};
            float lookSpeed{0.5f};
    };
}