#ifndef __GUI_HPP__
#define __GUI_HPP__

#include <stdint.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "input.hpp"
#include "math.hpp"
#include "memory.hpp"
#include "temporary_storage.hpp"

struct RendererState;

struct GuiVertex {
    Vec2f position;
    Vec4f color;
};

struct GuiState {
    GuiVertex* vertex_buffer;
    u32 current_size;
    
    Vec2f screen_size;
    
    GLFWwindow* window;
};

struct GuiResources {
    VkBuffer* buffers;
    AllocatedMemoryChunk* allocations;
};

bool init_gui(GuiState* gui_state, GuiResources* resources, RendererState* state);
void reset_gui(GuiState* state);
void cleanup_gui(GuiState* state, GuiResources* resources, RendererState* renderer_state, bool verbose = false);

void draw_rectangle(GuiState* state, f32 top, f32 left, f32 bottom, f32 right, Vec4f color);
bool draw_button(GuiState* state, Input* input, bool button_state, f32 top, f32 left, f32 bottom, f32 right, Vec4f color, Vec4f hover_color, Vec4f active_color);
char* to_string(GuiState* state, TemporaryStorage* storage, u32 indentation_level = 0);

#endif
