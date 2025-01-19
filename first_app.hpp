#pragma once
#include "ve_window.hpp"

#include <vector>
namespace ve {
    class FirstApp{
        public:
            static constexpr int WIDTH = 800;
            static constexpr int HEIGHT = 600;
            void run();
             
        private:
            VeWindow veWindow{WIDTH, HEIGHT, "First App"};

    };
}