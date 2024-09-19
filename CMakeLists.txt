cmake_minimum_required(VERSION 3.10)
project(Rasterizer)

find_package(OpenCV REQUIRED)

set(CMAKE_CXX_STANDARD 17)

# 包含 OpenCV 头文件的路径
include_directories(${OpenCV_INCLUDE_DIRS} "/opt/homebrew/Cellar/eigen/3.4.0_1/include/eigen3")

add_executable(Rasterizer main.cpp rasterizer.hpp rasterizer.cpp global.hpp Triangle.hpp Triangle.cpp)
target_link_libraries(Rasterizer ${OpenCV_LIBRARIES})
