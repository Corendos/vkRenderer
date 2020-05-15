#include "cg_input.h"

#include "cg_window_user_data.h"

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    WindowUserData* user_data = (WindowUserData*)glfwGetWindowUserPointer(window);
    
    if (key != -1) {
        user_data->input->changed = true;
        switch(action) {
            case GLFW_PRESS:
            user_data->input->key_pressed[key] = true;
            user_data->input->key_just_pressed[key] = true;
            break;
            case GLFW_RELEASE:
            user_data->input->key_pressed[key] = false;
            user_data->input->key_just_released[key] = true;
            break;
            default:
            break;
        }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    WindowUserData* user_data = (WindowUserData*)glfwGetWindowUserPointer(window);
    
    user_data->input->changed = true;
    switch(action) {
        case GLFW_PRESS:
        user_data->input->button_just_pressed[button] = true;
        user_data->input->button_pressed[button] = true;
        break;
        case GLFW_RELEASE:
        user_data->input->button_just_released[button] = true;
        user_data->input->button_pressed[button] = false;
        break;
        
    }
}

inline void reset_input(Input* input) {
    input->changed = false;
    input->mouse_delta_x = 0.0;
    input->mouse_delta_y = 0.0;
    
    for (int i = 0;i < GLFW_KEY_LAST + 1;++i) {
        input->key_just_pressed[i]  = false;
        input->key_just_released[i] = false;
    }
    
    for (int i = 0;i < GLFW_MOUSE_BUTTON_LAST + 1;++i) {
        input->button_just_pressed[i]  = false;
        input->button_just_released[i] = false;
    }
}
