#!/bin/bash

VULKAN_INCLUDE_DIR=/usr/include
GLFW_INCLUDE_DIR=/home/corendos/dev/lib/glfw-3.3.2/include
ASSIMP_INCLUDE_DIR=/home/corendos/dev/lib/assimp-5.0.1/include
VULKAN_LIB_DIR=/usr/lib/x86_64-linux-gnu
GLFW_LIB_DIR=/usr/local/lib
ASSIMP_LIB_DIR=/usr/local/lib

INCLUDE_DIRECTORY_FLAGS="-I../include -I$VULKAN_INCLUDE_DIR -I$GLFW_INCLUDE_DIR -I$ASSIMP_INCLUDE_DIR"
LIB_DIRECTORY_FLAGS="-L$VULKAN_LIB_DIR -L$GLFW_LIB_DIR -L$ASSIMP_LIB_DIR"
COMPILE_OPTIONS="-std=c++17 -fno-exceptions -O3"
LIBS="-lvulkan -lglfw3 -lassimp -lrt -lm -ldl -lX11 -lpthread"
DEFINES="-DPROGRAM_ROOT=\"$(pwd)\""

WORKING_DIR=$(readlink --canonicalize $(dirname "$0"))

cd $WORKING_DIR &&
echo "Building shaders" &&
./build_shaders.sh &&
echo "Building vk_renderer (Release)" &&
g++ ../src/main.cpp -o ../bin/Release/vk_renderer $INCLUDE_DIRECTORY_FLAGS $LIB_DIRECTORY_FLAGS $COMPILE_OPTIONS $LIBS $DEFINES
