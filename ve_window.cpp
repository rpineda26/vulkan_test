#include "ve_window.hpp"
 namespace ve {
     VeWindow::VeWindow(int w, int h, std::string name) : WIDTH(w), HEIGHT(h), windowName(name) {
         initWindow();
     }
     VeWindow::~VeWindow() {
         glfwDestroyWindow(window);
         glfwTerminate();
     }
     void VeWindow::initWindow() {
         glfwInit();
         glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
         glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
         window = glfwCreateWindow(WIDTH, HEIGHT, windowName.c_str(), nullptr, nullptr);
     }
\
 }