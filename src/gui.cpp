#include "gui.hpp"

#include <assert.h>

#include "macros.hpp"
#include "renderer.hpp"
#include "vk_helper.hpp"
#include "memory.hpp"

#define MAX_GUI_VERTEX_COUNT 10000 * 6

bool init_gui_state(GuiState* gui_state, RendererState* renderer_state) {
    gui_state->vertex_buffer = (GuiVertex*)calloc(MAX_GUI_VERTEX_COUNT, sizeof(GuiVertex));
    gui_state->current_size = 0;
    gui_state->screen_size.x = renderer_state->swapchain_extent.width;
    gui_state->screen_size.y = renderer_state->swapchain_extent.height;
    
    return true;
}

bool init_gui_resources(GuiResources* resources, RendererState* renderer_state) {
    resources->buffers = (VkBuffer*)calloc(renderer_state->swapchain_image_count, sizeof(VkBuffer));
    resources->allocations = (AllocatedMemoryChunk*)calloc(renderer_state->swapchain_image_count, sizeof(AllocatedMemoryChunk));
    
    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size  = MAX_GUI_VERTEX_COUNT * sizeof(GuiVertex);
    create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &renderer_state->selection.graphics_queue_family_index;
    
    for (int i = 0;i < renderer_state->swapchain_image_count;++i) {
        VkResult result = vkCreateBuffer(renderer_state->device, &create_info, nullptr, &resources->buffers[i]);
        if (result != VK_SUCCESS) {
            printf("vkCreateBuffer returned (%s)\n", vk_error_code_str(result));
            return false;
        }
        
        VkMemoryRequirements requirements = {};
        
        vkGetBufferMemoryRequirements(renderer_state->device, resources->buffers[i], &requirements);
        
        VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        
        if(!allocate(&renderer_state->memory_manager, renderer_state->device, requirements, memory_flags, &resources->allocations[i])) {
            return false;
        }
        
        result = vkBindBufferMemory(renderer_state->device, resources->buffers[i], resources->allocations[i].device_memory, resources->allocations[i].offset);
        if (result != VK_SUCCESS) {
            printf("vkBindBufferMemory returned (%s)\n", vk_error_code_str(result));
            return false;
        }
    }
    
    return true;
}

bool init_gui(GuiState* gui_state, GuiResources* resources, RendererState* renderer_state) {
    if (!init_gui_state(gui_state, renderer_state)) {
        printf("Failed to initialize gui state\n");
        return false;
    }
    if (!init_gui_resources(resources, renderer_state)) {
        printf("Failed to initialize gui resources\n");
        return false;
    }
    return true;
}

void reset_gui(GuiState* state) {
    state->current_size = 0;
}

void cleanup_gui(GuiState* gui_state, GuiResources* resources, RendererState* renderer_state, bool verbose) {
    if (gui_state->vertex_buffer) {
        if (verbose) {
            printf("Cleaning up GUI\n");
        }
        free_null(gui_state->vertex_buffer);
    }
    
    if (resources->buffers) {
        for(int i = 0;i < renderer_state->swapchain_image_count;++i) {
            if (verbose) {
                printf("Destroying GUI buffer (%p)\n", resources->buffers[i]);
            }
            vkDestroyBuffer(renderer_state->device, resources->buffers[i], nullptr);
        }
        free_null(resources->buffers);
    }
    
    if (resources->allocations) {
        for(int i = 0;i < renderer_state->swapchain_image_count;++i) {
            free(&renderer_state->memory_manager, &resources->allocations[i]);
        }
        free_null(resources->allocations);
    }
}

Vec2f screen_space_to_normalized_space(Vec2f screen_size, float x, float y) {
    Vec2f result = {};
    
    result.x = ((float)x / screen_size.x) * 2.0f - 1.0f;
    result.y = ((float)y / screen_size.y) * 2.0f - 1.0f;
    
    return result;
}

void draw_rectangle(GuiState* state, float top, float left, float bottom, float right, Vec4f color) {
    assert(state->current_size + 6 <= MAX_GUI_VERTEX_COUNT);
    
    GuiVertex bottom_left_vertex = {};
    bottom_left_vertex.position  = screen_space_to_normalized_space(state->screen_size, left, bottom);
    bottom_left_vertex.color     = color;
    
    GuiVertex top_left_vertex = {};
    top_left_vertex.position  = screen_space_to_normalized_space(state->screen_size, left, top);
    top_left_vertex.color     = color;
    
    GuiVertex bottom_right_vertex = {};
    bottom_right_vertex.position  = screen_space_to_normalized_space(state->screen_size, right, bottom);
    bottom_right_vertex.color     = color;
    
    GuiVertex top_right_vertex = {};
    top_right_vertex.position  = screen_space_to_normalized_space(state->screen_size, right, top);
    top_right_vertex.color     = color;
    
    state->vertex_buffer[state->current_size++] = bottom_left_vertex;
    state->vertex_buffer[state->current_size++] = top_left_vertex;
    state->vertex_buffer[state->current_size++] = top_right_vertex;
    
    state->vertex_buffer[state->current_size++] = bottom_left_vertex;
    state->vertex_buffer[state->current_size++] = top_right_vertex;
    state->vertex_buffer[state->current_size++] = bottom_right_vertex;
}

bool draw_button(GuiState* state, Input* input, bool button_state, float top, float left, float bottom, float right, Vec4f color, Vec4f hover_color, Vec4f active_color) {
    bool inside = input->mouse_x >= left &&
        input->mouse_x <= right &&
        input->mouse_y >= top &&
        input->mouse_y <= bottom;
    
    if (button_state) {
        draw_rectangle(state, top, left, bottom, right, active_color);
    } else if (inside) {
        draw_rectangle(state, top, left, bottom, right, hover_color);
    } else {
        draw_rectangle(state, top, left, bottom, right, color);
    }
    return (button_state && input->button_pressed[GLFW_MOUSE_BUTTON_LEFT]) || (inside && input->button_pressed[GLFW_MOUSE_BUTTON_LEFT]);
}

char* to_string(GuiState* state, TemporaryStorage* temporary_storage, uint32_t indentation_level) {
    char* indent_space = (char*)allocate(temporary_storage, indentation_level + 1);
    for (uint32_t i = 0;i < indentation_level;i++) {
        indent_space[i] = ' ';
    }
    indent_space[indentation_level] = 0;
    char* str = (char*)allocate(temporary_storage, 10000);
    
    sprintf(str,
            "GuiState {\n"
            "%s    vertex_buffer: %p\n"
            "%s    current_size: %d\n"
            "%s    screen_size: %s\n"
            "%s}",
            indent_space, state->vertex_buffer,
            indent_space, state->current_size,
            indent_space, to_string(state->screen_size, temporary_storage, indentation_level + 4),
            indent_space);
    
    return str;
}