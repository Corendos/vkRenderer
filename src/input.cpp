#include "input.hpp"

#include <iostream>

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

void reset_input(Input* input) {
    input->changed = false;

    for (int i = 0;i < GLFW_KEY_LAST;++i) {
	input->key_just_pressed[i] = false;
	input->key_just_released[i] = false;
    }
}
