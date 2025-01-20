#include "first_app.hpp"
#include "simple_render_system.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <array>
#include <stdexcept>
#include <cassert>
namespace ve {
    FirstApp::FirstApp() { loadGameObjects(); }
    FirstApp::~FirstApp() {}

    void FirstApp::run() {
        SimpleRenderSystem simpleRenderSystem{veDevice, veRenderer.getSwapChainRenderPass()};
        while (!veWindow.shouldClose()) {
            glfwPollEvents();
            if(auto commandBuffer = veRenderer.beginFrame()){
                veRenderer.beginSwapChainRenderPass(commandBuffer);
                simpleRenderSystem.renderGameObjects(commandBuffer,gameObjects);
                veRenderer.endSwapChainRenderPass(commandBuffer);
                veRenderer.endFrame();
            }
        }
        vkDeviceWaitIdle(veDevice.device()); //cpu wait for gpu to finish
    }
    void FirstApp::loadGameObjects() {
        std::vector<VeModel::Vertex> vertices{
            {{0.0f, -0.5f}, {1.0f,0.0f,0.0f}},
            {{0.5f, 0.5f},{0.0f,1.0f,0.0f}},
            {{-0.5f, 0.5f},{0.0f,0.0f,1.0f}}
        };
        auto veModel = std::make_shared<VeModel>(veDevice, vertices);
        auto triangle = VeGameObject::createGameObject();
        triangle.model = veModel;
        triangle.color = {0.1f, 0.8f, 0.1f};
        triangle.transform2d.translation.x = 0.2f;
        triangle.transform2d.scale = {2.0f,0.5f};
        triangle.transform2d.rotation = 0.25 * glm::two_pi<float>();
        gameObjects.push_back(std::move(triangle));
    }
}