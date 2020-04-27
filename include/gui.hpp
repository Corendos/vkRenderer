#ifndef __GUI_HPP__
#define __GUI_HPP__

#include <stdint.h>
#include <vulkan/vulkan.h>

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
    uint32_t current_size;
    
    Vec2f screen_size;
};

struct GuiResources {
    VkBuffer* buffers;
    AllocatedMemoryChunk* allocations;
};

bool init_gui(GuiState* gui_state, GuiResources* resources, RendererState* state);
void reset_gui(GuiState* state);
void cleanup_gui(GuiState* state, GuiResources* resources, RendererState* renderer_state, bool verbose = false);

void draw_rectangle(GuiState* state, float top, float left, float bottom, float right, Vec4f color);

char* to_string(GuiState* state, TemporaryStorage* storage, uint32_t indentation_level = 0);

#endif
