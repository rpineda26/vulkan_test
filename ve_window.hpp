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
            void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface); 
        private:
            void initWindow();
            std::string windowName;
            GLFWwindow *window;
            const int width, height;
    };
}