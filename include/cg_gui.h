#ifndef __GUI_H__
#define __GUI_H__

#include <stdint.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "cg_input.h"
#include "cg_math.h"
#include "cg_memory.h"
#include "cg_memory_arena.h"

#define MAX_GUI_VERTEX_COUNT 10000 * 6

struct RendererState;

struct GuiVertex {
    union {
        Vec2f position;
        Vec2i position_int;
    };
    Vec2f uv;
    Vec4f color;
    float text_blend;
    u32 font_index;
};

struct GuiDrawInfo {
    u32 start;
    u32 count;
    
    FontAtlas* font_atlas;
};

struct GuiDrawInfoEntry {
    GuiDrawInfo draw_info;
    
    GuiDrawInfoEntry* next;
};

struct GuiResources {
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkDescriptorSetLayout descriptor_set_layout;
    VkBuffer* buffers;
    AllocatedMemoryChunk* allocations;
    
    u32 font_atlas_slot_count;
    
    MemoryArena main_arena;
};

struct GuiState {
    GuiVertex* vertex_buffer;
    u32 current_size;
    
    Vec2u screen_size;
    
    GLFWwindow* window;
    GuiResources* resources;
};

enum TextAnchor {
    TopLeft,
    BottomLeft,
    TopRight,
    BottomRight,
    MiddleLeft,
    MiddleRight,
    TopMiddle,
    BottomMiddle,
    Center,
};

// Setup and destroy
bool create_gui_descriptor_set_layout(GuiResources* resources, RendererState* state);
bool create_gui_pipeline_layout(GuiResources* resources, RendererState* state);
bool create_gui_pipeline(GuiResources* resources, RendererState* state);
bool create_gui_buffers(GuiResources* resources, RendererState* state, MemoryArena* storage);
bool init_gui_resources(GuiResources* resources, RendererState* state);

void destroy_gui_buffers(GuiResources* resources, RendererState* state, bool verbose = false);
void destroy_gui_pipeline(GuiResources* resources, RendererState* state, bool verbose = false);
void destroy_gui_pipeline_layout(GuiResources* resources, RendererState* state, bool verbose = false);
void destroy_gui_descriptor_set_layout(GuiResources* resources, RendererState* state, bool verbose = false);
bool destroy_gui_resources(GuiResources* resources, RendererState* state, bool verbose = false);

bool init_gui_state(GuiState* gui_state, GuiResources* resources, RendererState* renderer_state, MemoryArena* storage);

bool init_gui(GuiState* gui_state, GuiResources* resources, RendererState* state);
void reset_gui(GuiState* state, GuiResources* resources);
void cleanup_gui(GuiState* state, GuiResources* resources, RendererState* renderer_state, bool verbose = false);

// Text
GuiVertex make_text_vertex(GuiState* state, FontAtlas* font_atlas,
                           i32 x, i32 y,
                           u32 u, u32 v,
                           u32 font_index,
                           Vec4f color);
GuiVertex make_text_vertex(GuiState* state, FontAtlas* font_atlas,
                           Vec2i position, Vec2u uv,
                           u32 font_index, Vec4f color);
Vec2u get_text_dimensions(ConstString* text, FontAtlas* font_atlas);
Rect2i get_text_bounding_box(ConstString* text, FontAtlas* font_atlas);
Vec2i compute_text_offset(Rect2i bounding_box, i32 x, i32 y, TextAnchor text_anchor);

char* to_string(GuiState* state, MemoryArena* temporary_storage, u32 indentation_level = 0);


// Public
void draw_rectangle(GuiState* state, i32 left, i32 top, i32 right, i32 bottom, Vec4f color);
void draw_rectangle(GuiState* state, Rect2i bound, Vec4f color);
void draw_text_rectangle(GuiState* state,
                         i32 left, i32 top, i32 right, i32 bottom,
                         ConstString* text,
                         FontAtlas* font_atlas,
                         Vec4f color, Vec4f text_color);
void draw_text_rectangle(GuiState* state,
                         Rect2i bound,
                         ConstString* text,
                         FontAtlas* font_atlas,
                         Vec4f color, Vec4f text_color);

bool draw_button(GuiState* state, Input* input,
                 i32 left, i32 top, i32 right, i32 bottom,
                 bool button_state,
                 Vec4f color,
                 Vec4f hover_color,
                 Vec4f active_color);
bool draw_button(GuiState* state, Input* input,
                 Rect2i bound,
                 bool button_state,
                 Vec4f color,
                 Vec4f hover_color,
                 Vec4f active_color);
bool draw_button(GuiState* state, Input* input,
                 Rect2i bound,
                 bool button_state,
                 Vec4f color,
                 Vec4f hover_color);

bool draw_text_button(GuiState* state, Input* input,
                      Rect2i bound,
                      ConstString* text,
                      FontAtlas* font_atlas,
                      bool button_state,
                      Vec4f color,
                      Vec4f hover_color,
                      Vec4f active_color,
                      Vec4f text_color);

void draw_text(GuiState* state, ConstString* text, i32 x, i32 y, Vec4f color, TextAnchor text_anchor, FontAtlas* font_atlas);

#endif
