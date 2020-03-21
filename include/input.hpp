#ifndef __INPUT_HPP__
#define __INPUT_HPP__

#include <GLFW/glfw3.h>

struct Input {
    bool changed;
    bool key_just_pressed[GLFW_KEY_LAST];
    bool key_just_released[GLFW_KEY_LAST];
    bool key_pressed[GLFW_KEY_LAST];
};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void reset_input(Input* input);

#endif
