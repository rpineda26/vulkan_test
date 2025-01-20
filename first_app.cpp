#include "first_app.hpp"
#include "simple_render_system.hpp"
#include "ve_camera.hpp"
#include "input_controller.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <array>
#include <stdexcept>
#include <cassert>
#include <chrono>
namespace ve {
    FirstApp::FirstApp() { loadGameObjects(); }
    FirstApp::~FirstApp() {}

    void FirstApp::run() {
        SimpleRenderSystem simpleRenderSystem{veDevice, veRenderer.getSwapChainRenderPass()};
        VeCamera camera{};
        auto viewerObject = VeGameObject::createGameObject();
        InputController inputController{};
        auto currentTime = std::chrono::high_resolution_clock::now();

        while (!veWindow.shouldClose()) {
            glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;
            frameTime = glm::min(frameTime, 0.1f); //clamp large frametimes
            inputController.moveInPlane(veWindow.getGLFWWindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);
            //setup viewing projection
            float aspect = veRenderer.getAspectRatio();
            // camera.setOrtho(-aspect, aspect, -0.9f, 0.9f, -1.0f, 1.0f);
            camera.setPerspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);

            if(auto commandBuffer = veRenderer.beginFrame()){
                veRenderer.beginSwapChainRenderPass(commandBuffer);
                simpleRenderSystem.renderGameObjects(commandBuffer,gameObjects,camera);
                veRenderer.endSwapChainRenderPass(commandBuffer);
                veRenderer.endFrame();
            }
        }
        vkDeviceWaitIdle(veDevice.device()); //cpu wait for gpu to finish
    }
    
    void FirstApp::loadGameObjects() {
        std::shared_ptr<VeModel> lveModel = VeModel::createModelFromFile(veDevice, "models/smooth_vase.obj");
        auto cube = VeGameObject::createGameObject();
        cube.model = lveModel;
        cube.transform.translation = {0.0f, 0.0f, 2.5f};
        cube.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back(std::move(cube));
    }
}