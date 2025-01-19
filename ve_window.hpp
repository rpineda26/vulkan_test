#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace ve {
    class VeWindow {
        public:
            VeWindow(int w, int h, std::string name);
            ~VeWindow();
            VeWindow(const VeWindow&) = delete;
            VeWindow& operator=(const VeWindow&) = delete;
            void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
            bool shouldClose() { return glfwWindowShouldClose(window); }
        private:
            void initWindow();
            const int WIDTH;
            const int HEIGHT;
            std::string windowName;
            GLFWwindow* window;
    };
}