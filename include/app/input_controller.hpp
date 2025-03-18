#pragma once

#include "ve_game_object.hpp"
#include "ve_window.hpp"
#include "frame_info.hpp"
#include "ve_game_object.hpp"
namespace ve{
    class InputController{
        public:
        struct KeyMappings{
            //macro keys for selecting objects
            int selectCamera{GLFW_KEY_1};
            int selectLight{GLFW_KEY_2};
            int selectVase{GLFW_KEY_3};
            int selectCube{GLFW_KEY_4};
            int selectFloor{GLFW_KEY_5};
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
            //change texture
            int nextTexture{GLFW_KEY_SPACE};
            //zoom
            // int zoomIn{GLFW_KEY_Z};
            // int zoomOut{GLFW_KEY_X};
        };
        struct MouseVariables{
            float dt, currTime, lastTime = 0.0;
            double lastMouseX, lastMouseY, mouseX, mouseY, mouseDistanceX, mouseDistanceY = 0.0;
            bool firstMouse = true;
        };
            void inputLogic(GLFWwindow* window, float deltaTime, VeGameObject::Map& gameObjects, VeGameObject& camera, int selectedObject);
            void moveInPlane(GLFWwindow* window, float deltaTime, VeGameObject& gameObject);
            //for testing lighting
            void mouseOffset(GLFWwindow* window, float deltaTime, VeGameObject& gameObject);
            KeyMappings keyMappings{};
            MouseVariables mouseVariables{};
            float moveSpeed{2.0f};
            float lookSpeed{2.0f};
            
        private:
            
    };

}