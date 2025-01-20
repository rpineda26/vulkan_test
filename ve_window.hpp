//Vulkan engine window header file
#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> 
#include <string>
namespace ve {
    class VeWindow {
        public:
            VeWindow(int w, int h, std::string name);
            ~VeWindow();
            VeWindow(const VeWindow &) = delete; 
            VeWindow &operator=(const VeWindow &) = delete;

            bool shouldClose() { return glfwWindowShouldClose(window); }
            VkExtent2D getExtent() { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }
            bool wasWindowResized() { return framebufferResized; }
            void resetWindowResizedFlag() { framebufferResized = false; }
            GLFWwindow *getGLFWWindow() { return window; }

            void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface); 
        private:
            static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
            void initWindow();
            std::string windowName;
            GLFWwindow *window;
            int width, height;
            bool framebufferResized = false;
    };
}