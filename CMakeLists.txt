cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

project(vk_renderer)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

link_directories(/usr/local/lib/)

# Set compile options
add_compile_options(-std=c++17 -pthread -fno-exceptions -O3)

add_subdirectory(src)
