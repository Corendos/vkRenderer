#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <GLFW/glfw3.h>

void window_resize_callback(GLFWwindow* window, int new_width, int new_height);
void framebuffer_resize_callback(GLFWwindow* window, int new_width, int new_height);

#endif
