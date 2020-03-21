#include "window.hpp"

#include <iostream>

#include "window_user_data.hpp"

void window_resize_callback(GLFWwindow* window, int new_width, int new_height) {
    std::cout << "New Size: width: " << new_width << "  height: " << new_height << std::endl;

    WindowUserData* user_data = (WindowUserData*)glfwGetWindowUserPointer(window);

    user_data->swapchain_need_recreation = true;
}

void framebuffer_resize_callback(GLFWwindow* window, int new_width, int new_height) {
    std::cout << "New framebuffer size: width: " << new_width << "  height: " << new_height << std::endl;
}
