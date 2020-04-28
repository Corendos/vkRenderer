#include "input.hpp"

#include "window_user_data.hpp"

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

void mouse_position_callback(GLFWwindow* window, f64 mouse_x, f64 mouse_y) {
    WindowUserData* user_data = (WindowUserData*)glfwGetWindowUserPointer(window);
    
    user_data->input->changed = true;
    
    f64 last_mouse_x = user_data->input->mouse_x;
    f64 last_mouse_y = user_data->input->mouse_y;
    
    user_data->input->mouse_x = mouse_x;
    user_data->input->mouse_y = mouse_y;
    
    if (user_data->input->not_first_delta) {
        user_data->input->mouse_delta_x= user_data->input->mouse_x - last_mouse_x;
        user_data->input->mouse_delta_y = user_data->input->mouse_y - last_mouse_y;
        printf("Delta x: %06.6f Delta y: %06.6f\n",
               user_data->input->mouse_delta_x,
               user_data->input->mouse_delta_y);
    } else {
        user_data->input->not_first_delta = true;
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

void reset_input(Input* input) {
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
