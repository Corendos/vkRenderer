#ifndef __INPUT_H__
#define __INPUT_H__

#include <GLFW/glfw3.h>

struct Input {
    bool changed;
    bool key_just_pressed[GLFW_KEY_LAST + 1];
    bool key_just_released[GLFW_KEY_LAST + 1];
    bool key_pressed[GLFW_KEY_LAST + 1];
    
    bool button_just_pressed[GLFW_MOUSE_BUTTON_LAST + 1];
    bool button_just_released[GLFW_MOUSE_BUTTON_LAST + 1];
    bool button_pressed[GLFW_MOUSE_BUTTON_LAST + 1];
    
    f64 mouse_x;
    f64 mouse_y;
    
    f64 mouse_delta_x;
    f64 mouse_delta_y;
    
    bool not_first_delta;
};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_position_callback(GLFWwindow* window, f64 xpos, f64 ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void reset_input(Input* input);

#endif
