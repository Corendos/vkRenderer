#ifndef __INPUT_HPP__
#define __INPUT_HPP__

#include <GLFW/glfw3.h>

struct Input {
    bool changed;
    bool key_just_pressed[GLFW_KEY_LAST + 1];
    bool key_just_released[GLFW_KEY_LAST + 1];
    bool key_pressed[GLFW_KEY_LAST + 1];

    bool button_just_pressed[GLFW_MOUSE_BUTTON_LAST + 1];
    bool button_just_released[GLFW_MOUSE_BUTTON_LAST + 1];
    bool button_pressed[GLFW_MOUSE_BUTTON_LAST + 1];

    double mouse_x;
    double mouse_y;

    double mouse_delta_x;
    double mouse_delta_y;

    bool not_first_delta;
};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_position_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void reset_input(Input* input);

#endif
