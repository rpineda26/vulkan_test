#include "input_controller.hpp"

namespace ve{
    void InputController::inputLogic(GLFWwindow* window, float deltaTime, VeGameObject::Map& gameObjects, VeGameObject& camera, glm::vec3& lightPosition, SelectedObject& selectedObject){
        if(glfwGetKey(window, keyMappings.selectCamera) == GLFW_PRESS){
            selectedObject = CAMERA;
        }
        if(glfwGetKey(window, keyMappings.selectLight) == GLFW_PRESS){
            selectedObject = LIGHT;
        }
        if(glfwGetKey(window, keyMappings.selectVase) == GLFW_PRESS){
            selectedObject = VASE;
        }
        if(glfwGetKey(window, keyMappings.selectCube) == GLFW_PRESS){
            selectedObject = CUBE;
        }
        if(glfwGetKey(window, keyMappings.selectFloor) == GLFW_PRESS){
            selectedObject = FLOOR;
        }
        
        switch(selectedObject){
            case CAMERA:
                moveInPlane(window, deltaTime, camera);
                break;
            case LIGHT:
                movePosition(window, deltaTime, lightPosition);
                break;
            case VASE:
                moveInPlane(window, deltaTime, gameObjects.at(0));
                break;
            case CUBE:
                moveInPlane(window, deltaTime, gameObjects.at(1));
                break;
            case FLOOR:
                moveInPlane(window, deltaTime, gameObjects.at(2));
                break;
            default:
                break;
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
        rotate.y += mouseVariables.mouseDistanceX;
        rotate.x -= mouseVariables.mouseDistanceY;
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
    void InputController::movePosition(GLFWwindow* window, float deltaTime, glm::vec3& position) {
        glm::vec3 movement{0.0f};
        glm::vec3 pivot{0.0f, 0.0f, 0.0f};  // Default pivot at the origin (you can adjust this)

        // Movement controls
        if (glfwGetKey(window, keyMappings.forward) == GLFW_PRESS) {
            movement.z -= 1.0f;  // Move forward
        }
        if (glfwGetKey(window, keyMappings.backward) == GLFW_PRESS) {
            movement.z += 1.0f;  // Move backward
        }
        if (glfwGetKey(window, keyMappings.leftward) == GLFW_PRESS) {
            movement.x -= 1.0f;  // Move left
        }
        if (glfwGetKey(window, keyMappings.rightward) == GLFW_PRESS) {
            movement.x += 1.0f;  // Move right
        }
        if (glfwGetKey(window, keyMappings.upward) == GLFW_PRESS) {
            movement.y += 1.0f;  // Move up
        }
        if (glfwGetKey(window, keyMappings.downward) == GLFW_PRESS) {
            movement.y -= 1.0f;  // Move down
        }

        // Apply movement
        if (glm::dot(movement, movement) > std::numeric_limits<float>::epsilon()) {
            position += glm::normalize(movement) * moveSpeed * deltaTime;
        }

        // Rotation controls
        if (glfwGetKey(window, keyMappings.tiltLeft) == GLFW_PRESS || glfwGetKey(window, keyMappings.tiltRight) == GLFW_PRESS ||
            glfwGetKey(window, keyMappings.tiltUp) == GLFW_PRESS || glfwGetKey(window, keyMappings.tiltDown) == GLFW_PRESS) {

            // Calculate rotation angles
            float yaw = 0.0f;  // Rotation around Y-axis
            if (glfwGetKey(window, keyMappings.tiltLeft) == GLFW_PRESS) {
                yaw += lookSpeed * deltaTime;
            }
            if (glfwGetKey(window, keyMappings.tiltRight) == GLFW_PRESS) {
                yaw -= lookSpeed * deltaTime;
            }

            float pitch = 0.0f;  // Rotation around X-axis
            if (glfwGetKey(window, keyMappings.tiltUp) == GLFW_PRESS) {
                pitch += lookSpeed * deltaTime;
            }
            if (glfwGetKey(window, keyMappings.tiltDown) == GLFW_PRESS) {
                pitch -= lookSpeed * deltaTime;
            }

            // Transform the position relative to the pivot
            position -= pivot;

            // Apply rotations in order: pitch (X-axis), then yaw (Y-axis)
            glm::mat4 pitchMatrix = glm::rotate(glm::mat4(1.0f), pitch, glm::vec3(1.0f, 0.0f, 0.0f));
            glm::mat4 yawMatrix = glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0.0f, 1.0f, 0.0f));

            glm::vec4 rotatedPosition = yawMatrix * pitchMatrix * glm::vec4(position, 1.0f);
            position = glm::vec3(rotatedPosition);

            // Return to the original pivot
            position += pivot;
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