#include "input_controller.hpp"
#include <imgui.h>

namespace ve{
    void InputController::inputLogic(GLFWwindow* window, float deltaTime, VeGameObject::Map& gameObjects, VeGameObject& camera, int selectedObject){
        ImGuiIO& io = ImGui::GetIO();
        if(ImGui::IsAnyItemActive()){
            return;
        }
        
        if(selectedObject == -1){
            moveInPlane(window, deltaTime, camera);
        }else{
            moveInPlane(window, deltaTime, gameObjects.at(selectedObject));
        }
    }
    void InputController::moveInPlane(GLFWwindow* window, float deltaTime, VeGameObject& gameObject){
        glm::vec3 rotate{0.0f};
        //compute keyboard input contribution
        if(glfwGetKey(window, keyMappings.tiltUp) == GLFW_PRESS){
            rotate.x += 1.0f;
        }
        if(glfwGetKey(window, keyMappings.tiltDown) == GLFW_PRESS){
            rotate.x -= 1.0f;
        }
        if(glfwGetKey(window, keyMappings.tiltLeft) == GLFW_PRESS){
            rotate.y -= 1.0f;
        }
        if(glfwGetKey(window, keyMappings.tiltRight) == GLFW_PRESS){
            rotate.y += 1.0f;
        }
        //mouse contribution

        mouseOffset(window, deltaTime, gameObject);
        //temporarily disable mouse control
        // rotate.y += mouseVariables.mouseDistanceX; 
        // rotate.x -= mouseVariables.mouseDistanceY;

        //calculate rotation
        if(glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()){
            gameObject.transform.rotation += glm::normalize(rotate) * lookSpeed * deltaTime;
        }
        gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -glm::half_pi<float>(), glm::half_pi<float>());
        gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());

        float yaw = gameObject.transform.rotation.y;
        const glm::vec3 forward{sin(yaw), 0.0f, glm::cos(yaw)};
        const glm::vec3 right{forward.z, 0.0f, -forward.x};
        const glm::vec3 up{0.0f, -1.0f, 0.0f};

        //movement
        glm::vec3 movement{0.0f};
        if(glfwGetKey(window, keyMappings.forward) == GLFW_PRESS){
            movement += forward;
        }
        if(glfwGetKey(window, keyMappings.backward) == GLFW_PRESS){
            movement -= forward;
        }
        if(glfwGetKey(window, keyMappings.leftward) == GLFW_PRESS){
            movement -= right;
        }
        if(glfwGetKey(window, keyMappings.rightward) == GLFW_PRESS){
            movement += right;
        }
        if(glfwGetKey(window, keyMappings.upward) == GLFW_PRESS){
            movement += up;
        }
        if(glfwGetKey(window, keyMappings.downward) == GLFW_PRESS){
            movement -= up;
        }
        if(glm::dot(movement, movement) > std::numeric_limits<float>::epsilon()){
            gameObject.transform.translation += glm::normalize(movement) * moveSpeed * deltaTime;
        }
    }
   
    void InputController::mouseOffset(GLFWwindow* window, float deltaTime, VeGameObject& gameObject){
        glfwGetCursorPos(window, &mouseVariables.mouseX, &mouseVariables.mouseY);
        if(mouseVariables.firstMouse){
            mouseVariables.lastMouseX = mouseVariables.mouseX;
            mouseVariables.lastMouseY = mouseVariables.mouseY;
            mouseVariables.firstMouse = false;
        }
        
        mouseVariables.mouseDistanceX = mouseVariables.mouseX - mouseVariables.lastMouseX;
        mouseVariables.mouseDistanceY = mouseVariables.mouseY - mouseVariables.lastMouseY;

        mouseVariables.lastMouseX = mouseVariables.mouseX;
        mouseVariables.lastMouseY = mouseVariables.mouseY;
    }

}