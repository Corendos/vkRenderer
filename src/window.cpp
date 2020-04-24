#include "window.hpp"

#include "window_user_data.hpp"

void window_resize_callback(GLFWwindow* window, int new_width, int new_height) {
    printf("New Size: width: %d  height: %d\n", new_width, new_height);
    
    WindowUserData* user_data = (WindowUserData*)glfwGetWindowUserPointer(window);
    
    user_data->swapchain_need_recreation = true;
}

void framebuffer_resize_callback(GLFWwindow* window, int new_width, int new_height) {
    printf("New framebuffer size: width: %d  height: %d\n", new_width, new_height);
}
