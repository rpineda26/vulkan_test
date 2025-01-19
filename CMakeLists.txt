cmake_minimum_required(VERSION 3.10)

# Project name and version
project(MyProject VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
if(DEFINED ENV{VULKAN_SDK_PATH})
    include_directories($ENV{VULKAN_SDK_PATH}/include)
    link_directories($ENV{VULKAN_SDK_PATH}/lib)
endif()

# Find glfw3 via pkg-config

find_package(GLFW REQUIRED)
if (GLFW_FOUND)
  message(STATUS "Glfw found")
  include_directories(${GLFW_INCLUDE_DIRS})
  link_libraries (${GLFW_LIBRARIES})
endif (GLFW_FOUND)


# Add the executable
add_executable(a.out main.cpp)

# Link libraries
target_link_libraries(a.out PRIVATE ${GLFW3_LIBRARIES} Vulkan)
