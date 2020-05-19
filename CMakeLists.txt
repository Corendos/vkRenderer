cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

project(vk_renderer)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(assimp REQUIRED)

link_directories(/usr/local/lib/)

# Set compile options
add_compile_options(-std=c++17 -pthread -fno-exceptions)

if (CMAKE_BUILD_TYPE MATCHES Release)
	message("Optimization enabled")
	add_compile_options(-O3 -DNDEBUG)
endif(CMAKE_BUILD_TYPE MATCHES Release)

add_subdirectory(src)
