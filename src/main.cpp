#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <time.h>
#include <unistd.h>

//#define OBJ_CUSTOM

#ifdef NDEBUG
#warning "No debug mode"
#endif

#include "cg_types.h"

#include "cg_benchmark.h"
#include "cg_camera.h"
#include "cg_color.h"
#include "cg_files.h"
#include "cg_fonts.h"
#include "cg_gui.h"
#include "cg_hash.h"
#include "cg_input.h"
#include "cg_macros.h"
#include "cg_memory.h"
#include "cg_obj_loader.h"
#include "cg_renderer.h"
#include "cg_shaders.h"
#include "cg_string.h"
#include "cg_material.h"
#include "cg_memory_arena.h"
#include "cg_random.h"
#include "cg_texture.h"
#include "cg_temporary_memory.h"
#include "cg_timer.h"
#include "cg_utils.h"
#include "cg_vertex.h"
#include "cg_vk_helper.h"
#include "cg_window.h"
#include "cg_window_user_data.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#undef STB_IMAGE_WRITE_IMPLEMENTATION

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#undef STB_RECT_PACK_IMPLEMENTATION

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#undef STB_TRUETYPE_IMPLEMENTATION


#include "cg_benchmark.cpp"
#include "cg_color.cpp"
#include "cg_files.cpp"
#include "cg_fonts.cpp"
#include "cg_gui.cpp"
#include "cg_hash.cpp"
#include "cg_input.cpp"
#include "cg_math.cpp"
#include "cg_memory.cpp"
#include "cg_obj_loader.cpp"
#include "cg_shaders.cpp"
#include "cg_string.cpp"
#include "cg_material.cpp"
#include "cg_memory_arena.cpp"
#include "cg_random.cpp"
#include "cg_texture.cpp"
#include "cg_temporary_memory.cpp"
#include "cg_timer.cpp"
#include "cg_utils.cpp"
#include "cg_vk_helper.cpp"
#include "cg_vertex.cpp"
#include "cg_window.cpp"


struct FpsCounter {
    u64 start;
    u64 end;
    u64 cumulated_frame_duration;
    u32 frame_count;
};

struct Time {
    u64 start;
    u64 end;
    f64 delta_time;
    f64 cumulated_time;
};

inline void do_mouse_input(RendererState* state, Input* input) {
    f64 last_mouse_x = input->mouse_x;
    f64 last_mouse_y = input->mouse_y;
    
    glfwGetCursorPos(state->window, &input->mouse_x, &input->mouse_y);
    
    if (input->not_first_delta) {
        input->mouse_delta_x = input->mouse_x - state->swapchain_extent.width / 2;
        input->mouse_delta_y = input->mouse_y - state->swapchain_extent.height / 2;
    } else {
        input->not_first_delta = true;
    }
    
    if (state->cursor_locked) {
        glfwSetCursorPos(state->window,
                         state->swapchain_extent.width / 2,
                         state->swapchain_extent.height / 2);
    }
}

inline void do_input(RendererState* state, Input* input) {
    reset_input(input);
    glfwPollEvents();
    do_mouse_input(state, input);
}

inline void update_fps_counter(RendererState* state, GLFWwindow* window, FpsCounter* fps_counter) {
    fps_counter->end = get_time_ns();
    u64 last_start = fps_counter->start;
    fps_counter->start = get_time_ns();
    
    fps_counter->cumulated_frame_duration += fps_counter->end - last_start;
    fps_counter->frame_count++;
    
    TemporaryMemory temporary_memory = make_temporary_memory(&state->main_arena);
    
    if (fps_counter->frame_count == state->temp_data.frame_count_update) {
        f64 average_frame_duration = (f64)fps_counter->cumulated_frame_duration / (f64)fps_counter->frame_count;
        average_frame_duration /= 1000000000.0;
        
        fps_counter->frame_count = 0;
        fps_counter->cumulated_frame_duration = 0;
        
        String temp = push_string(&temporary_memory, 1000);
        
        string_format(temp, "%f fps", 1.0 / average_frame_duration);
        glfwSetWindowTitle(window, temp.str);
        
        u32 int_fps = 1.0 / average_frame_duration;
        state->temp_data.frame_count_update = int_fps;
    }
    
    char* data = (char*)allocate(&temporary_memory, 100000000);
    
    destroy_temporary_memory(&temporary_memory);
}


inline bool handle_swapchain_recreation(RendererState* state, WindowUserData* window_user_data) {
    if (window_user_data->swapchain_need_recreation) {
        // Wait for the last frame in flight to be ready
        VkResult result = vkQueueWaitIdle(state->graphics_queue);
        if (result != VK_SUCCESS) {
            println("vkQueueWaitIdle returned (%s)", vk_error_code_str(result));
            return false;
        }
        
        result = vkQueueWaitIdle(state->present_queue);
        if (result != VK_SUCCESS) {
            println("vkQueueWaitIdle returned (%s)", vk_error_code_str(result));
            return false;
        }
        
        // Destroy the pipeline
        destroy_semaphores(state);
        destroy_framebuffers(state);
        destroy_pipeline(state);
        destroy_gui_pipeline(&state->gui_resources, state);
        destroy_depth_images(state);
        
        if (!create_swapchain(state)) {
            println("Failed to recreate swapchain");
            return false;
        }
        
        free_null(state->swapchain_images);
        if (!get_swapchain_images(state)) {
            println("Failed to get swapchain images");
            return false;
        }
        
        destroy_swapchain_image_views(state);
        if (!create_swapchain_image_views(state)) {
            println("Failed to create swapchain image views");
            return false;
        }
        
        if (!create_depth_images(state)) {
            println("Failed to create depth image");
            return false;
        }
        
        if (!create_semaphore(state)) {
            println("Failed to create semaphore");
            return false;
        }
        
        if(!create_graphics_pipeline(state)) {
            println("Failed to recreate pipeline");
        }
        
        if(!create_gui_pipeline(&state->gui_resources, state)) {
            println("Failed to recreate gui pipeline");
        }
        
        if(!create_framebuffers(state)) {
            println("Failed to recreate framebuffers");
        }
        
        window_user_data->swapchain_need_recreation = false;
        
        state->camera.aspect = (f32)state->swapchain_extent.width / (f32)state->swapchain_extent.height;
        state->camera.context.projection = perspective(state->camera.fov, state->camera.aspect, 0.1f, 100.0f);
        state->gui_state.screen_size = new_vec2u(state->swapchain_extent.width, state->swapchain_extent.height);
        
        state->skip_image = false;
    }
    
    return true;
}


inline bool create_entity(RendererState* state, Vertex* vertex_buffer, u32 vertex_buffer_size, u32* entity_id) {
    if (state->entity_count == MAX_ENTITY_COUNT) return false;
    
    u32 buffer_size = vertex_buffer_size * sizeof(Vertex);
    
    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = buffer_size;
    create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &state->selection.graphics_queue_family_index;
    
    Entity* entity = &state->entities[state->entity_count];
    
    VkResult result = vkCreateBuffer(state->device, &create_info, nullptr, &entity->buffer);
    if (result != VK_SUCCESS) {
        println("vkCreateBuffer returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    VkMemoryRequirements requirements = {};
    
    vkGetBufferMemoryRequirements(state->device, entity->buffer, &requirements);
    
    VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    
    if(!allocate(&state->memory_manager, state->device, requirements, memory_flags, &entity->allocation)) {
        return false;
    }
    
    result = vkBindBufferMemory(state->device, entity->buffer, entity->allocation.device_memory, entity->allocation.offset);
    if (result != VK_SUCCESS) {
        println("vkBindBufferMemory returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    memcpy(entity->allocation.data, vertex_buffer, buffer_size);
    
    entity->offset = state->entity_count * sizeof(EntityTransformData);
    entity->transform_data = &state->entity_resources.transform_data[state->entity_count];
    entity->transform_data->model_matrix = identity_mat4f();
    entity->transform_data->normal_matrix = identity_mat4f();
    entity->size = vertex_buffer_size;
    entity->id = state->entity_count + 1;
    *entity_id = entity->id;
    state->entity_count++;
    
    return true;
}

inline bool create_square_entity(RendererState* state) {
    f32 z = 0.0f;
    Vertex vertex_buffer[6] = {};
    vertex_buffer[0].position = new_vec3f(-0.5, -0.5, z);
    vertex_buffer[0].color    = new_vec3f(1.0, 0.0, 0.0);
    vertex_buffer[1].position = new_vec3f(-0.5, 0.5, z);
    vertex_buffer[1].color    = new_vec3f(0.0, 1.0, 0.0);
    vertex_buffer[2].position = new_vec3f(0.5, -0.5, z);
    vertex_buffer[2].color    = new_vec3f(0.0, 0.0, 1.0);
    
    vertex_buffer[3].position = new_vec3f(-0.5, 0.5, z);
    vertex_buffer[3].color    = new_vec3f(0.0, 1.0, 0.0);
    vertex_buffer[4].position = new_vec3f(0.5, 0.5, z);
    vertex_buffer[4].color    = new_vec3f(1.0, 1.0, 1.0);
    vertex_buffer[5].position = new_vec3f(0.5, -0.5, z);
    vertex_buffer[5].color    = new_vec3f(0.0, 0.0, 1.0);
    
    u32 entity_id = 0;
    
    return create_entity(state, vertex_buffer, array_size(vertex_buffer), &entity_id);
}

inline void create_cube(Vec3f size, Vertex* vertices) {
    // Front face
    vertices[ 0].position = new_vec3f(-size.x, -size.y,  size.z);
    vertices[ 1].position = new_vec3f(-size.x,  size.y,  size.z);
    vertices[ 2].position = new_vec3f( size.x, -size.y,  size.z);
    
    vertices[ 3].position = new_vec3f( size.x, -size.y,  size.z);
    vertices[ 4].position = new_vec3f(-size.x,  size.y,  size.z);
    vertices[ 5].position = new_vec3f( size.x,  size.y,  size.z);
    
    vertices[ 0].color = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[ 1].color = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[ 2].color = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[ 3].color = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[ 4].color = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[ 5].color = new_vec3f(1.0f, 0.0f, 0.0f);
    
    vertices[ 0].normal = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[ 1].normal = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[ 2].normal = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[ 3].normal = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[ 4].normal = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[ 5].normal = new_vec3f(0.0f, 0.0f, 1.0f);
    
    // Right face
    vertices[ 6].position = new_vec3f( size.x, -size.y,  size.z);
    vertices[ 7].position = new_vec3f( size.x,  size.y,  size.z);
    vertices[ 8].position = new_vec3f( size.x, -size.y, -size.z);
    
    vertices[ 9].position = new_vec3f( size.x, -size.y, -size.z);
    vertices[10].position = new_vec3f( size.x,  size.y,  size.z);
    vertices[11].position = new_vec3f( size.x,  size.y, -size.z);
    
    vertices[ 6].color = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[ 7].color = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[ 8].color = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[ 9].color = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[10].color = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[11].color = new_vec3f(0.0f, 1.0f, 0.0f);
    
    vertices[ 6].normal = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[ 7].normal = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[ 8].normal = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[ 9].normal = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[10].normal = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[11].normal = new_vec3f(1.0f, 0.0f, 0.0f);
    
    // Back face
    vertices[12].position = new_vec3f( size.x, -size.y, -size.z);
    vertices[13].position = new_vec3f( size.x,  size.y, -size.z);
    vertices[14].position = new_vec3f(-size.x, -size.y, -size.z);
    
    vertices[15].position = new_vec3f(-size.x, -size.y, -size.z);
    vertices[16].position = new_vec3f( size.x,  size.y, -size.z);
    vertices[17].position = new_vec3f(-size.x,  size.y, -size.z);
    
    vertices[12].color = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[13].color = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[14].color = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[15].color = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[16].color = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[17].color = new_vec3f(0.0f, 0.0f, 1.0f);
    
    vertices[12].normal = new_vec3f(0.0f, 0.0f, -1.0f);
    vertices[13].normal = new_vec3f(0.0f, 0.0f, -1.0f);
    vertices[14].normal = new_vec3f(0.0f, 0.0f, -1.0f);
    vertices[15].normal = new_vec3f(0.0f, 0.0f, -1.0f);
    vertices[16].normal = new_vec3f(0.0f, 0.0f, -1.0f);
    vertices[17].normal = new_vec3f(0.0f, 0.0f, -1.0f);
    
    // Left face
    vertices[18].position = new_vec3f(-size.x, -size.y, -size.z);
    vertices[19].position = new_vec3f(-size.x,  size.y, -size.z);
    vertices[20].position = new_vec3f(-size.x, -size.y,  size.z);
    
    vertices[21].position = new_vec3f(-size.x, -size.y,  size.z);
    vertices[22].position = new_vec3f(-size.x,  size.y, -size.z);
    vertices[23].position = new_vec3f(-size.x,  size.y,  size.z);
    
    vertices[18].color = new_vec3f(1.0f, 0.0f, 1.0f);
    vertices[19].color = new_vec3f(1.0f, 0.0f, 1.0f);
    vertices[20].color = new_vec3f(1.0f, 0.0f, 1.0f);
    vertices[21].color = new_vec3f(1.0f, 0.0f, 1.0f);
    vertices[22].color = new_vec3f(1.0f, 0.0f, 1.0f);
    vertices[23].color = new_vec3f(1.0f, 0.0f, 1.0f);
    
    vertices[18].normal = new_vec3f(-1.0f, 0.0f, 0.0f);
    vertices[19].normal = new_vec3f(-1.0f, 0.0f, 0.0f);
    vertices[20].normal = new_vec3f(-1.0f, 0.0f, 0.0f);
    vertices[21].normal = new_vec3f(-1.0f, 0.0f, 0.0f);
    vertices[22].normal = new_vec3f(-1.0f, 0.0f, 0.0f);
    vertices[23].normal = new_vec3f(-1.0f, 0.0f, 0.0f);
    
    // Top face
    vertices[24].position = new_vec3f( size.x,  size.y,  size.z);
    vertices[25].position = new_vec3f(-size.x,  size.y,  size.z);
    vertices[26].position = new_vec3f( size.x,  size.y, -size.z);
    
    vertices[27].position = new_vec3f( size.x,  size.y, -size.z);
    vertices[28].position = new_vec3f(-size.x,  size.y,  size.z);
    vertices[29].position = new_vec3f(-size.x,  size.y, -size.z);
    
    vertices[24].color = new_vec3f(1.0f, 1.0f, 0.0f);
    vertices[25].color = new_vec3f(1.0f, 1.0f, 0.0f);
    vertices[26].color = new_vec3f(1.0f, 1.0f, 0.0f);
    vertices[27].color = new_vec3f(1.0f, 1.0f, 0.0f);
    vertices[28].color = new_vec3f(1.0f, 1.0f, 0.0f);
    vertices[29].color = new_vec3f(1.0f, 1.0f, 0.0f);
    
    vertices[24].normal = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[25].normal = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[26].normal = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[27].normal = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[28].normal = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[29].normal = new_vec3f(0.0f, 1.0f, 0.0f);
    
    // Bottom face
    vertices[30].position = new_vec3f(-size.x, -size.y,  size.z);
    vertices[31].position = new_vec3f( size.x, -size.y,  size.z);
    vertices[32].position = new_vec3f( size.x, -size.y, -size.z);
    
    vertices[33].position = new_vec3f( size.x, -size.y, -size.z);
    vertices[34].position = new_vec3f(-size.x, -size.y, -size.z);
    vertices[35].position = new_vec3f(-size.x, -size.y,  size.z);
    
    vertices[30].color = new_vec3f(0.0f, 1.0f, 1.0f);
    vertices[31].color = new_vec3f(0.0f, 1.0f, 1.0f);
    vertices[32].color = new_vec3f(0.0f, 1.0f, 1.0f);
    vertices[33].color = new_vec3f(0.0f, 1.0f, 1.0f);
    vertices[34].color = new_vec3f(0.0f, 1.0f, 1.0f);
    vertices[35].color = new_vec3f(0.0f, 1.0f, 1.0f);
    
    vertices[30].normal = new_vec3f(0.0f, -1.0f, 0.0f);
    vertices[31].normal = new_vec3f(0.0f, -1.0f, 0.0f);
    vertices[32].normal = new_vec3f(0.0f, -1.0f, 0.0f);
    vertices[33].normal = new_vec3f(0.0f, -1.0f, 0.0f);
    vertices[34].normal = new_vec3f(0.0f, -1.0f, 0.0f);
    vertices[35].normal = new_vec3f(0.0f, -1.0f, 0.0f);
}

inline void create_cube(Vec3f size, Vec3f color, Vertex* vertices) {
    // Front face
    vertices[ 0].position = new_vec3f(-size.x, -size.y,  size.z);
    vertices[ 1].position = new_vec3f(-size.x,  size.y,  size.z);
    vertices[ 2].position = new_vec3f( size.x, -size.y,  size.z);
    
    vertices[ 3].position = new_vec3f( size.x, -size.y,  size.z);
    vertices[ 4].position = new_vec3f(-size.x,  size.y,  size.z);
    vertices[ 5].position = new_vec3f( size.x,  size.y,  size.z);
    
    vertices[ 0].color = color;
    vertices[ 1].color = color;
    vertices[ 2].color = color;
    vertices[ 3].color = color;
    vertices[ 4].color = color;
    vertices[ 5].color = color;
    
    vertices[ 0].normal = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[ 1].normal = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[ 2].normal = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[ 3].normal = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[ 4].normal = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[ 5].normal = new_vec3f(0.0f, 0.0f, 1.0f);
    
    // Right face
    vertices[ 6].position = new_vec3f( size.x, -size.y,  size.z);
    vertices[ 7].position = new_vec3f( size.x,  size.y,  size.z);
    vertices[ 8].position = new_vec3f( size.x, -size.y, -size.z);
    
    vertices[ 9].position = new_vec3f( size.x, -size.y, -size.z);
    vertices[10].position = new_vec3f( size.x,  size.y,  size.z);
    vertices[11].position = new_vec3f( size.x,  size.y, -size.z);
    
    vertices[ 6].color = color;
    vertices[ 7].color = color;
    vertices[ 8].color = color;
    vertices[ 9].color = color;
    vertices[10].color = color;
    vertices[11].color = color;
    
    vertices[ 6].normal = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[ 7].normal = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[ 8].normal = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[ 9].normal = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[10].normal = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[11].normal = new_vec3f(1.0f, 0.0f, 0.0f);
    
    // Back face
    vertices[12].position = new_vec3f( size.x, -size.y, -size.z);
    vertices[13].position = new_vec3f( size.x,  size.y, -size.z);
    vertices[14].position = new_vec3f(-size.x, -size.y, -size.z);
    
    vertices[15].position = new_vec3f(-size.x, -size.y, -size.z);
    vertices[16].position = new_vec3f( size.x,  size.y, -size.z);
    vertices[17].position = new_vec3f(-size.x,  size.y, -size.z);
    
    vertices[12].color = color;
    vertices[13].color = color;
    vertices[14].color = color;
    vertices[15].color = color;
    vertices[16].color = color;
    vertices[17].color = color;
    
    vertices[12].normal = new_vec3f(0.0f, 0.0f, -1.0f);
    vertices[13].normal = new_vec3f(0.0f, 0.0f, -1.0f);
    vertices[14].normal = new_vec3f(0.0f, 0.0f, -1.0f);
    vertices[15].normal = new_vec3f(0.0f, 0.0f, -1.0f);
    vertices[16].normal = new_vec3f(0.0f, 0.0f, -1.0f);
    vertices[17].normal = new_vec3f(0.0f, 0.0f, -1.0f);
    
    // Left face
    vertices[18].position = new_vec3f(-size.x, -size.y, -size.z);
    vertices[19].position = new_vec3f(-size.x,  size.y, -size.z);
    vertices[20].position = new_vec3f(-size.x, -size.y,  size.z);
    
    vertices[21].position = new_vec3f(-size.x, -size.y,  size.z);
    vertices[22].position = new_vec3f(-size.x,  size.y, -size.z);
    vertices[23].position = new_vec3f(-size.x,  size.y,  size.z);
    
    vertices[18].color = color;
    vertices[19].color = color;
    vertices[20].color = color;
    vertices[21].color = color;
    vertices[22].color = color;
    vertices[23].color = color;
    
    vertices[18].normal = new_vec3f(-1.0f, 0.0f, 0.0f);
    vertices[19].normal = new_vec3f(-1.0f, 0.0f, 0.0f);
    vertices[20].normal = new_vec3f(-1.0f, 0.0f, 0.0f);
    vertices[21].normal = new_vec3f(-1.0f, 0.0f, 0.0f);
    vertices[22].normal = new_vec3f(-1.0f, 0.0f, 0.0f);
    vertices[23].normal = new_vec3f(-1.0f, 0.0f, 0.0f);
    
    // Top face
    vertices[24].position = new_vec3f( size.x,  size.y,  size.z);
    vertices[25].position = new_vec3f(-size.x,  size.y,  size.z);
    vertices[26].position = new_vec3f( size.x,  size.y, -size.z);
    
    vertices[27].position = new_vec3f( size.x,  size.y, -size.z);
    vertices[28].position = new_vec3f(-size.x,  size.y,  size.z);
    vertices[29].position = new_vec3f(-size.x,  size.y, -size.z);
    
    vertices[24].color = color;
    vertices[25].color = color;
    vertices[26].color = color;
    vertices[27].color = color;
    vertices[28].color = color;
    vertices[29].color = color;
    
    vertices[24].normal = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[25].normal = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[26].normal = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[27].normal = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[28].normal = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[29].normal = new_vec3f(0.0f, 1.0f, 0.0f);
    
    // Bottom face
    vertices[30].position = new_vec3f(-size.x, -size.y,  size.z);
    vertices[31].position = new_vec3f( size.x, -size.y,  size.z);
    vertices[32].position = new_vec3f( size.x, -size.y, -size.z);
    
    vertices[33].position = new_vec3f( size.x, -size.y, -size.z);
    vertices[34].position = new_vec3f(-size.x, -size.y, -size.z);
    vertices[35].position = new_vec3f(-size.x, -size.y,  size.z);
    
    vertices[30].color = color;
    vertices[31].color = color;
    vertices[32].color = color;
    vertices[33].color = color;
    vertices[34].color = color;
    vertices[35].color = color;
    
    vertices[30].normal = new_vec3f(0.0f, -1.0f, 0.0f);
    vertices[31].normal = new_vec3f(0.0f, -1.0f, 0.0f);
    vertices[32].normal = new_vec3f(0.0f, -1.0f, 0.0f);
    vertices[33].normal = new_vec3f(0.0f, -1.0f, 0.0f);
    vertices[34].normal = new_vec3f(0.0f, -1.0f, 0.0f);
    vertices[35].normal = new_vec3f(0.0f, -1.0f, 0.0f);
}

inline Mat4f random_translation_matrix(f32 range) {
    return translation_matrix(2.0f * randf() * range - range,
                              2.0f * randf() * range - range,
                              2.0f * randf() * range - range);
}

inline Mat4f random_rotation_matrix() {
    return rotation_matrix(2.0f * PI * randf(),
                           2.0f * PI * randf(),
                           2.0f * PI * randf());
}

inline bool create_cube_entity(RendererState* state, Mat4f transform_matrix) {
    Vertex vertex_buffer[36] = {};
    create_cube(new_vec3f(0.2f, 0.2f, 0.2f), vertex_buffer);
    
    u32 entity_id = 0;
    
    if(!create_entity(state, vertex_buffer, array_size(vertex_buffer), &entity_id)) {
        return false;
    }
    
    Entity* entity = &state->entities[entity_id - 1];
    
    entity->transform_data->model_matrix = transform_matrix;
    entity->transform_data->normal_matrix = transpose_inverse(&entity->transform_data->model_matrix);
    
    return true;
}

inline bool create_cube_entity(RendererState* state) {
    Mat4f t = random_translation_matrix(5.0f);
    Mat4f r = random_rotation_matrix();
    return create_cube_entity(state, t * r);
}

inline bool create_cube_entity(RendererState* state, Vec3f position) {
    return create_cube_entity(state, translation_matrix(position.x, position.y, position.z));
}

inline bool create_cube_entity_color(RendererState* state, Mat4f transform_matrix, Vec3f color) {
    Vertex vertex_buffer[36] = {};
    create_cube(new_vec3f(0.2f, 0.2f, 0.2f), color, vertex_buffer);
    
    u32 entity_id = 0;
    
    if(!create_entity(state, vertex_buffer, array_size(vertex_buffer), &entity_id)) {
        return false;
    }
    
    Entity* entity = &state->entities[entity_id - 1];
    
    entity->transform_data->model_matrix = transform_matrix;
    entity->transform_data->normal_matrix = transpose_inverse(&entity->transform_data->model_matrix);
    
    return true;
}

inline bool create_cube_entity_color(RendererState* state, Vec3f position, Vec3f color) {
    return create_cube_entity_color(state, translation_matrix(position.x, position.y, position.z), color);
}

inline bool allocate_camera_descriptor_sets(RendererState* state) {
    state->camera_resources.descriptor_sets = (VkDescriptorSet*)calloc(state->swapchain_image_count, sizeof(VkDescriptorSet));
    
    VkDescriptorSetLayout* layouts;
    layouts = (VkDescriptorSetLayout*)calloc(state->swapchain_image_count, sizeof(VkDescriptorSetLayout));
    for (int i = 0;i < state->swapchain_image_count;++i) {
        layouts[i] = state->descriptor_set_layouts[CameraDescriptorSetLayout];
    }
    
    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = state->descriptor_pool;
    allocate_info.descriptorSetCount = state->swapchain_image_count;
    allocate_info.pSetLayouts = layouts;
    
    VkResult result = vkAllocateDescriptorSets(state->device, &allocate_info, state->camera_resources.descriptor_sets);
    if (result != VK_SUCCESS) {
        println("vkAllocateDescriptorSets returned (%s)", vk_error_code_str(result));
        return false;
        free_null(layouts);
    }
    
    free_null(layouts);
    
    return true;
}

inline bool create_camera_buffers(RendererState* state) {
    state->camera_resources.buffers     = (VkBuffer*)            calloc(state->swapchain_image_count, sizeof(VkBuffer));
    state->camera_resources.allocations = (AllocatedMemoryChunk*)calloc(state->swapchain_image_count, sizeof(AllocatedMemoryChunk));
    
    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = sizeof(CameraContext);
    create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &state->selection.graphics_queue_family_index;
    
    for (int i = 0;i < state->swapchain_image_count;++i) {
        VkResult result = vkCreateBuffer(state->device, &create_info, nullptr, &state->camera_resources.buffers[i]);
        if (result != VK_SUCCESS) {
            println("vkCreateBuffer returned (%s)", vk_error_code_str(result));
            return false;
        }
        
        VkMemoryRequirements requirements = {};
        
        vkGetBufferMemoryRequirements(state->device, state->camera_resources.buffers[i], &requirements);
        
        VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        
        if(!allocate(&state->memory_manager, state->device, requirements, memory_flags, &state->camera_resources.allocations[i])) {
            return false;
        }
        
        result = vkBindBufferMemory(state->device, state->camera_resources.buffers[i], state->camera_resources.allocations[i].device_memory, state->camera_resources.allocations[i].offset);
        if (result != VK_SUCCESS) {
            println("vkBindBufferMemory returned (%s)",vk_error_code_str(result));
            return false;
        }
        
        memcpy(state->camera_resources.allocations[i].data, &state->camera.context, sizeof(CameraContext));
    }
    
    return true;
}

inline void update_camera_descriptor_set(RendererState* state) {
    VkDescriptorBufferInfo* buffer_info;
    
    buffer_info = (VkDescriptorBufferInfo*)calloc(state->swapchain_image_count, sizeof(VkDescriptorBufferInfo));
    
    for (int i = 0;i < state->swapchain_image_count;++i) {
        buffer_info[i].buffer = state->camera_resources.buffers[i];
        buffer_info[i].offset = 0;
        buffer_info[i].range = sizeof(CameraContext);
    }
    
    VkWriteDescriptorSet* writes;
    
    writes = (VkWriteDescriptorSet*)calloc(state->swapchain_image_count, sizeof(VkWriteDescriptorSet));
    
    for (int i = 0;i < state->swapchain_image_count;++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = state->camera_resources.descriptor_sets[i];
        writes[i].dstBinding = 0;
        writes[i].dstArrayElement = 0;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[i].pBufferInfo = &buffer_info[i];
    }
    
    vkUpdateDescriptorSets(state->device, state->swapchain_image_count, writes, 0, nullptr);
    
    free_null(buffer_info);
    free_null(writes);
}

inline bool init_camera(RendererState* state) {
    state->camera.fov = 70.0f;
    state->camera.position = &state->camera.context.view_position;
    *state->camera.position = new_vec3f(0.0f, 0.0f, 0.0f);
    state->camera.yaw = -PI_2;
    state->camera.pitch = 0.0f;
    state->camera.mouse_sensitivity = 0.0015f;
    state->camera.speed= 1.0f;
    state->camera.boost_speed = state->camera.speed * 5.0f;
    state->camera.aspect = (f32)state->swapchain_extent.width / (f32)state->swapchain_extent.height;
    
    state->camera.context.projection = perspective(state->camera.fov, state->camera.aspect, 0.1f, 100.0f);
    state->camera.context.view = look_from_yaw_and_pitch(*state->camera.position, state->camera.yaw, state->camera.pitch, new_vec3f(0.0f, 1.0f, 0.0f));
    
    if (!allocate_camera_descriptor_sets(state)) {
        return false;
    }
    
    if (!create_camera_buffers(state)) {
        return false;
    }
    
    update_camera_descriptor_set(state);
    
    return true;
}

inline bool create_entity_buffers(RendererState* state) {
    state->entity_resources.buffers = (VkBuffer*)calloc(state->swapchain_image_count, sizeof(VkBuffer));
    state->entity_resources.allocations = (AllocatedMemoryChunk*)calloc(state->swapchain_image_count, sizeof(AllocatedMemoryChunk));
    
    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size  = MAX_ENTITY_COUNT * sizeof(EntityTransformData);
    create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &state->selection.graphics_queue_family_index;
    
    for (int i = 0;i < state->swapchain_image_count;++i) {
        VkResult result = vkCreateBuffer(state->device, &create_info, nullptr, &state->entity_resources.buffers[i]);
        if (result != VK_SUCCESS) {
            println("vkCreateBuffer returned (%s)", vk_error_code_str(result));
            return false;
        }
        
        VkMemoryRequirements requirements = {};
        
        vkGetBufferMemoryRequirements(state->device, state->entity_resources.buffers[i], &requirements);
        
        VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        
        if(!allocate(&state->memory_manager, state->device, requirements, memory_flags, &state->entity_resources.allocations[i])) {
            return false;
        }
        
        result = vkBindBufferMemory(state->device, state->entity_resources.buffers[i], state->entity_resources.allocations[i].device_memory, state->entity_resources.allocations[i].offset);
        if (result != VK_SUCCESS) {
            println("vkBindBufferMemory returned (%s)", vk_error_code_str(result));
            return false;
        }
    }
    
    return true;
}

inline bool create_entity_descriptor_sets(RendererState* state) {
    state->entity_resources.descriptor_sets = (VkDescriptorSet*)calloc(state->swapchain_image_count, sizeof(VkDescriptorSet));
    
    VkDescriptorSetLayout* layouts;
    layouts = (VkDescriptorSetLayout*)calloc(state->swapchain_image_count, sizeof(VkDescriptorSetLayout));
    for (int i = 0;i < state->swapchain_image_count;++i) {
        layouts[i] = state->descriptor_set_layouts[TransformDescriptorSetLayout];
    }
    
    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = state->descriptor_pool;
    allocate_info.descriptorSetCount = state->swapchain_image_count;
    allocate_info.pSetLayouts = layouts;
    
    VkResult result = vkAllocateDescriptorSets(state->device, &allocate_info, state->entity_resources.descriptor_sets);
    if (result != VK_SUCCESS) {
        println("vkAllocateDescriptorSets returned (%s)", vk_error_code_str(result));
        return false;
        free_null(layouts);
    }
    
    free_null(layouts);
    
    return true;
}

inline void update_entity_descriptor_sets(RendererState* state) {
    VkDescriptorBufferInfo* buffer_info;
    
    buffer_info = (VkDescriptorBufferInfo*)calloc(state->swapchain_image_count, sizeof(VkDescriptorBufferInfo));
    
    for (int i = 0;i < state->swapchain_image_count;++i) {
        buffer_info[i].buffer = state->entity_resources.buffers[i];
        buffer_info[i].offset = 0;
        buffer_info[i].range = sizeof(EntityTransformData);
    }
    
    VkWriteDescriptorSet* writes;
    
    writes = (VkWriteDescriptorSet*)calloc(state->swapchain_image_count, sizeof(VkWriteDescriptorSet));
    
    for (int i = 0;i < state->swapchain_image_count;++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = state->entity_resources.descriptor_sets[i];
        writes[i].dstBinding = 0;
        writes[i].dstArrayElement = 0;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        writes[i].pBufferInfo = &buffer_info[i];
    }
    
    vkUpdateDescriptorSets(state->device, state->swapchain_image_count, writes, 0, nullptr);
    
    free_null(buffer_info);
    free_null(writes);
}

inline bool init_entities(RendererState* state) {
    if (!create_entity_buffers(state)) {
        println("Error: failed to create entity buffers");
        return false;
    }
    
    if (!create_entity_descriptor_sets(state)) {
        println("Error: failed to create entity descriptor sets");
        return false;
    }
    
    update_entity_descriptor_sets(state);
    
    return true;
}

inline bool init_renderer(RendererState* state) {
    if (!glfwInit()) {
        println("Error: failed to initialize glfw library");
        return false;
    } else {
        println("glfw init: success");
    }
    
    if (!glfwVulkanSupported()) {
        println("Vulkan is not supported");
        return false;
    } else {
        println("vulkan: supported");
    }
    
    if (!create_instance(&state->instance)) {
        return false;
    } else {
        println("instance init: success");
    }
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    state->window = glfwCreateWindow(1280, 720, "vkRenderer", nullptr, nullptr);
    
    if (!state->window) {
        println("Error: failed to create window");
        return false;
    } else {
        println("window init: success");
    }
    
    VkResult result = glfwCreateWindowSurface(state->instance, state->window, nullptr, &state->surface);
    if(result != VK_SUCCESS) {
        println("Error: failed to create window surface (%s)", vk_error_code_str(result));
        return false;
    } else {
        println("surface init: success");
    }
    
    u32 extension_count = 0;
    const char** required_instance_extensions = glfwGetRequiredInstanceExtensions(&extension_count);
    
    if (!check_required_instance_extensions(required_instance_extensions, extension_count)) {
        return false;
    } else {
        println("required extensions: supported");
    }
    
    if (!select_physical_device(state)) {
        println("Error: failed to select a physical device.");
        return false;
    } else {
        println("physical device selection: success");
    }
    
    if (!check_required_device_extensions(state->selection.device)) {
        return false;
    } else {
        println("required device extensions: supported");
    }
    
    if (!create_device_and_queues(state)) {
        println("Error: failed to create the device.");
        return false;
    } else {
        println("device and queues init: success");
    }
    
    u32 allocation_size = MB(128);
    u32 min_page_size = KB(4);
    
    if (!init_memory(&state->memory_manager, allocation_size, min_page_size, state->selection.device)) {
        return false;
    } else {
        println("memory manager init: success");
    }
    
    if (!select_surface_format(state)) {
        return false;
    }
    
    if (!select_present_mode(state)) {
        return false;
    }
    
    if (!create_swapchain(state)) {
        return false;
    } else {
        println("swapchain init: success");
    }
    
    if (!get_swapchain_images(state)) {
        return false;
    } else {
        println("swapchain images init: success");
    }
    
    if (!create_depth_images(state)) {
        return false;
    } else {
        println("depth images init: success");
    }
    
    if (!create_semaphore(state)) {
        return false;
    }
    
    if (!create_fences(state)) {
        return false;
    }
    
    if (!create_command_pool(state)) {
        return false;
    } else {
        println("command pool init: success");
    }
    
    if (!create_swapchain_image_views(state)) {
        return false;
    } else {
        println("swapchain image view init: success");
    }
    
    if (!init_shader_catalog(&state->shader_catalog, 64)) {
        return false;
    } else {
        println("shader catalog init: success");
    }
    
    if (!load_shader_module("resources/shaders/basic.vert.spv", "basic.vert", state->device, &state->shader_catalog)) {
        return false;
    } else {
        println("basic vertex shader init: success");
    }
    
    if (!load_shader_module("resources/shaders/basic.frag.spv", "basic.frag", state->device, &state->shader_catalog)) {
        return false;
    } else {
        println("basic fragment shader init: success");
    }
    
    if (!load_shader_module("resources/shaders/gui.vert.spv", "gui.vert", state->device, &state->shader_catalog)) {
        return false;
    } else {
        println("gui vertex shader init: success");
    }
    
    if (!load_shader_module("resources/shaders/gui.frag.spv", "gui.frag", state->device, &state->shader_catalog)) {
        return false;
    } else {
        println("gui fragment shader init: success");
    }
    
    if (!create_descriptor_set_layout(state)) {
        return false;
    } else {
        println("descriptor set layout init: success");
    }
    
    if (!create_descriptor_pool(state)) {
        return false;
    } else {
        println("descriptor pool init: success");
    }
    
    if (!create_pipeline_layout(state)) {
        return false;
    } else {
        println("pipeline layout init: success");
    }
    
    if (!create_render_pass(state)) {
        return false;
    } else {
        println("render pass init: success");
    }
    
    if (!create_graphics_pipeline(state)) {
        return false;
    } else {
        println("graphics pipeline init: success");
    }
    
    if (!create_framebuffers(state)) {
        return false;
    } else {
        println("framebuffers init: success");
    }
    
    return true;
}

inline bool init_font(RendererState* state) {
    if (!init_font_catalog(&state->font_catalog, 32, &state->main_arena)) {
        return false;
    } else {
        println("font catalog init: success");
    }
    
    ConstString font_name = make_literal_string("ubuntu");
    
    String font_file_path = push_string(&state->temporary_storage, 256);
    string_format(font_file_path, "%s/resources/fonts/UbuntuMono-R.ttf", PROGRAM_ROOT);
    
    ConstString const_font_file_path = make_const_string(&font_file_path);
    
    if (!load_and_add_font_to_catalog(&state->font_catalog, &font_name, &const_font_file_path, &state->main_arena)) {
        return false;
    } else {
        println("loading '%s' font: success", font_name.str);
    }
    
    if (!init_font_atlas_catalog(&state->font_atlas_catalog, state->gui_resources.font_atlas_slot_count, state, &state->main_arena)) {
        return false;
    } else {
        println("init font atlas catalog : success");
    }
    
    u32 font_sizes[] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 30};
    
    if (!create_and_add_font_atlases(&state->font_catalog, &font_name, font_sizes, array_size(font_sizes), &state->font_atlas_catalog, 32, 95, state, &state->main_arena)) {
        return false;
    } else {
        println("create font atlases: success");
    }
    
    return true;
}

inline bool init(RendererState* state, WindowUserData* window_user_data) {
    init_random();
    if (!init_memory_arena(&state->temporary_storage)) {
        println("Error: failed to initialize temporary_storage");
        return false;
    } else {
        println("temporary_storage init : success");
    }
    
    if (!init_memory_arena(&state->main_arena, MAIN_ARENA_SIZE)) {
        println("Error: failed to initialize main arena");
        return false;
    } else {
        println("main arena init : success");
    }
    
    if (!init_renderer(state)) {
        return false;
    } else {
        println("renderer init: success");
    }
    
    Vec2f screen_size = {};
    screen_size.x = (f32)state->swapchain_extent.width;
    screen_size.y = (f32)state->swapchain_extent.height;
    if (!init_gui(&state->gui_state, &state->gui_resources, state)) {
        return false;
    } else {
        println("gui init: success");
    }
    
    if (!init_texture_catalog(&state->texture_catalog, 32, state)) {
        return false;
    } else {
        println("texture catalog init: success");
    }
    
    if (!init_font(state)) {
        return false;
    } else {
        println("font init: success");
    }
    
    if (!init_material_catalog(&state->material_catalog, state)) {
        return false;
    } else {
        println("material catalog init : success");
    }
    
    u64 start = get_time_ns();
    if (!add_named_material(&state->material_catalog, {}, make_literal_string("steel_material"))) {
        return false;
    }
    
    if (!add_named_material(&state->material_catalog, {}, make_literal_string("concrete_material"))) {
        return false;
    }
    
    if (!add_named_material(&state->material_catalog, {}, make_literal_string("grass_material"))) {
        return false;
    }
    u64 end = get_time_ns();
    println("Building material catalog took %lu ns", (end - start));
    
    println("Current entries:");
    for (u32 i = 0;i < state->material_catalog.entry_count;++i) {
        MaterialEntry* entry = state->material_catalog.material_entries + i;
        println("    %s: %u", entry->material_name.str, entry->index);
    }
    
    ConstString material_name = make_literal_string("steel_material");
    i32 material_index = get_named_material_index(&state->material_catalog, material_name);
    if (material_index < 0) {
        println("Error: failed to get '%s' material", material_name.str);
        return false;
    }
    
    if (!init_camera(state)) {
        return false;
    } else {
        println("camera init : success");
    }
    
    if (!init_entities(state)) {
        return false;
    } else {
        println("entities init : success");
    }
    
    if (!create_cube_entity(state, new_vec3f(1.0f, 0.0f, 0.0f))) {
        return false;
    }
    if (!create_cube_entity(state, new_vec3f(-1.0f, 0.0f, 0.0f))) {
        return false;
    }
    if (!create_cube_entity(state, new_vec3f(0.0f, 0.0f, 1.0f))) {
        return false;
    }
    if (!create_cube_entity(state, new_vec3f(0.0f, 0.0f, -1.0f))) {
        return false;
    }
    
    if (!create_cube_entity_color(state, new_vec3f(0.0f, 0.0f, 0.0f), new_vec3f(1.0f, 1.0f, 1.0f))) {
        return false;
    }
    
    state->temp_data.light_entity_id = state->entity_count - 1;
    
    start = get_time_ns();
    String obj_filename_var = push_string(&state->temporary_storage, 100);
    string_format(obj_filename_var, "%s/resources/models/obj/Trumpet.obj", PROGRAM_ROOT);
    ConstString obj_filename = make_const_string(&obj_filename_var);
    
    Vertex* vertex_buffer = {};
    u32 vertex_buffer_size = 0;
    
    if (!load_obj_file(&obj_filename, &vertex_buffer, &vertex_buffer_size, &state->main_arena)) {
        return false;
    }
    end = get_time_ns();
    
    println("Loading took %f s", (f32)(end - start) / (f32)1e9);
    
    u32 trumpet_id = 0;
    if (!create_entity(state, vertex_buffer, vertex_buffer_size, &trumpet_id)) {
        return false;
    }
    
    state->entities[state->entity_count - 1].transform_data->model_matrix = translation_matrix(0.0f, 3.0f, 0.0f);
    
    glfwSetWindowUserPointer(state->window, window_user_data);
    glfwSetKeyCallback(state->window, key_callback);
    glfwSetMouseButtonCallback(state->window, mouse_button_callback);
    glfwSetWindowSizeCallback(state->window, window_resize_callback);
    glfwSetFramebufferSizeCallback(state->window, framebuffer_resize_callback);
    
    state->cursor_locked = true;
    glfwSetInputMode(state->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(state->window, state->swapchain_extent.width / 2, state->swapchain_extent.height / 2);
    
    state->temp_data.frame_count_update = 60;
    
    return true;
}

inline bool wait_for_acquire_fence(RendererState* state) {
    VkResult result = vkWaitForFences(state->device,
                                      1,
                                      &state->acquire_fences[state->current_semaphore_index],
                                      VK_TRUE,
                                      1000000000);
    if (result != VK_SUCCESS) {
        println("vkWaitForFences returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    result = vkResetFences(state->device, 1, &state->acquire_fences[state->current_semaphore_index]);
    if (result != VK_SUCCESS) {
        println("vkResetFences returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline bool wait_for_submit_fence(RendererState* state) {
    VkResult result = vkWaitForFences(state->device, 1, state->submissions[state->image_index].fence, VK_TRUE, 1000000000);
    if (result != VK_SUCCESS) {
        println("vkWaitForFences returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    result = vkResetFences(state->device, 1, state->submissions[state->image_index].fence);
    if (result != VK_SUCCESS) {
        println("vkResetFences returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline void update_time(Time* time) {
    time->end = get_time_ns();
    time->delta_time = (f64)(time->end - time->start) / (f64)1e9;
    time->cumulated_time += time->delta_time;
    time->start = get_time_ns();
}

inline void update_camera(RendererState* state, Input* input, Time* time) {
    Camera* camera = &state->camera;
    
    if (state->cursor_locked) {
        camera->yaw -= input->mouse_delta_x * camera->mouse_sensitivity;
        camera->pitch += input->mouse_delta_y * camera->mouse_sensitivity;
    }
    
    if (camera->yaw > 2.0 * PI) {
        camera->yaw -= 2.0 * PI;
    } else if (camera->yaw < -2.0 * PI) {
        camera->yaw += 2.0 * PI;
    }
    camera->pitch = clamp(camera->pitch, - PI_2, PI_2);
    
    Vec3f forward = new_vec3f(-sin(camera->yaw) * cos(camera->pitch), -sin(camera->pitch), -cos(camera->yaw) * cos(camera->pitch));
    Vec3f side = new_vec3f(cos(camera->yaw), 0.0f, -sin(camera->yaw));
    
    Vec3f move_vector = new_vec3f();
    
    f32 camera_speed = 0.0f;
    if (input->key_pressed[GLFW_KEY_LEFT_SHIFT]) {
        camera_speed = camera->boost_speed;
    } else {
        camera_speed = camera->speed;
    }
    
    if (input->key_pressed[GLFW_KEY_W]) {
        move_vector = move_vector + forward;
    }
    if (input->key_pressed[GLFW_KEY_S]) {
        move_vector = move_vector - forward;
    }
    if (input->key_pressed[GLFW_KEY_D]) {
        move_vector = move_vector + side;
    }
    if (input->key_pressed[GLFW_KEY_A]) {
        move_vector = move_vector - side;
    }
    
    f64 delta_time = time->delta_time;
    
    move_vector = normalize(&move_vector);
    move_vector.x = move_vector.x * camera_speed * delta_time;
    move_vector.y = move_vector.y * camera_speed * delta_time;
    move_vector.z = move_vector.z * camera_speed * delta_time;
    
    *camera->position = *camera->position + move_vector;
    camera->context.view = look_from_yaw_and_pitch(*camera->position, camera->yaw, camera->pitch, new_vec3f(0.0f, 1.0f, 0.0f));
    
    f32 rotation_speed = (f32)state->temp_data.rotation_speed;
    
    f32* angle = &state->temp_data.current_angle;
    *angle += rotation_speed * delta_time * 0.25f;
    
    camera->context.light_position = new_vec3f(3.0f * cos(2.0f * PI * *angle),
                                               0.0f,
                                               3.0f * sin(2.0f * PI * *angle));
}

inline void update_entities(RendererState* state) {
    // Update light cube position
    Vec3f* light_position = &state->camera.context.light_position;
    Entity* light_entity = &state->entities[state->temp_data.light_entity_id];
    Mat4f* light_model_matrix = &light_entity->transform_data->model_matrix;
    *light_model_matrix = translation_matrix(light_position->x, light_position->y, light_position->z);
    Mat4f* light_normal_matrix = &light_entity->transform_data->normal_matrix;
    *light_normal_matrix = transpose_inverse(light_model_matrix);
    
    memcpy(state->entity_resources.allocations[state->image_index].data, state->entity_resources.transform_data, state->entity_count * sizeof(EntityTransformData));
}

inline void update_gui(RendererState* state, Input* input) {
    reset_gui(&state->gui_state, &state->gui_resources);
    
    GuiState* gui_state = &state->gui_state;
    
    Vec4f color = new_coloru(30, 30, 30);
    Vec4f hover_color = new_coloru(40, 40, 40);
    Vec4f active_color = new_coloru(10, 10, 10);
    Vec4f text_color = new_coloru(200, 200, 200);
    
    ConstString font_name = make_literal_string("ubuntu");
    FontAtlas* font_atlas = get_font_atlas_from_catalog(&state->font_atlas_catalog, &font_name, 20);
    assert(font_atlas != 0);
    
    char temp[101] = {};
    String var_text = make_string(temp, 100);
    
    string_format(var_text, "<");
    ConstString text = make_const_string(&var_text);
    bool minus_button_status = draw_text_button(gui_state, input,
                                                new_rect2i_dim(10, 10, 30, 30),
                                                &text,
                                                font_atlas,
                                                state->temp_data.minus_button_status,
                                                color, hover_color, active_color,
                                                text_color);
    if (minus_button_status) {
        if (!state->temp_data.minus_button_status) {
            state->temp_data.minus_button_status = true;
        }
    } else {
        if (state->temp_data.minus_button_status) {
            state->temp_data.rotation_speed -= 1;
            state->temp_data.minus_button_status = false;
        }
    }
    
    string_format(var_text, "Current Speed: %i", state->temp_data.rotation_speed);
    text = make_const_string(&var_text);
    draw_text_rectangle(gui_state, new_rect2i_dim(45, 10, 200, 30), &text, font_atlas, color, text_color);
    
    string_format(var_text, ">");
    text = make_const_string(&var_text);
    bool plus_button_status = draw_text_button(gui_state, input,
                                               new_rect2i_dim(250, 10, 30, 30),
                                               &text,
                                               font_atlas,
                                               state->temp_data.plus_button_status,
                                               color, hover_color, active_color,
                                               text_color);
    
    if (plus_button_status) {
        if (!state->temp_data.plus_button_status) {
            state->temp_data.plus_button_status = true;
        }
    } else {
        if (state->temp_data.plus_button_status) {
            state->temp_data.rotation_speed += 1;
            state->temp_data.plus_button_status = false;
        }
    }
    
    memcpy(state->gui_resources.allocations[state->image_index].data, state->gui_state.vertex_buffer, state->gui_state.current_size * sizeof(GuiVertex));
}

inline void update(RendererState* state, Input* input, Time* time) {
    update_gui(state, input);
    update_camera(state, input, time);
    update_entities(state);
    memcpy(state->camera_resources.allocations[state->image_index].data, &state->camera.context, sizeof(CameraContext));
    
    if (input->button_just_pressed[GLFW_MOUSE_BUTTON_RIGHT]) {
        state->cursor_locked = !state->cursor_locked;
        
        if (state->cursor_locked) {
            glfwSetInputMode(state->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetCursorPos(state->window, state->swapchain_extent.width / 2, state->swapchain_extent.height / 2);
        } else {
            glfwSetInputMode(state->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            //glfwSetInputMode(state->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }
    }
    
    if (input->button_just_pressed[GLFW_MOUSE_BUTTON_MIDDLE]) {
        glfwSetWindowShouldClose(state->window, GLFW_TRUE);
    }
    
    if (input->key_just_pressed[GLFW_KEY_Y]) {
        state->temp_data.counter = 0;
        state->temp_data.updater = 1;
    }
}

inline VkResult render(RendererState* state) {
    VkCommandBuffer command_buffer = {};
    
    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = state->command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    
    VkResult allocate_result = vkAllocateCommandBuffers(state->device, &allocate_info, &command_buffer);
    if (allocate_result != VK_SUCCESS) {
        println("vkAllocateCommandBuffers returned (%s)", vk_error_code_str(allocate_result));
        return allocate_result;
    }
    
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    VkResult result = vkBeginCommandBuffer(command_buffer, &begin_info);
    if (result != VK_SUCCESS) {
        println("vkBeginCommandBuffer returned (%s)", vk_error_code_str(result));
        return result;
    }
    
    VkClearValue clear_colors[2] = {};
    clear_colors[0].color = {0.05f, 0.05f, 0.05f, 1.0f};
    clear_colors[1].depthStencil = {1.0f, 0};
    
    VkRenderPassBeginInfo renderpass_begin_info = {};
    renderpass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpass_begin_info.renderPass = state->renderpass;
    renderpass_begin_info.framebuffer = state->framebuffers[state->image_index];
    renderpass_begin_info.renderArea.offset = {0, 0};
    renderpass_begin_info.renderArea.extent = state->swapchain_extent;
    renderpass_begin_info.clearValueCount = array_size(clear_colors);
    renderpass_begin_info.pClearValues = clear_colors;
    
    vkCmdBeginRenderPass(command_buffer, &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->pipeline);
    VkDeviceSize offset = 0;
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->pipeline_layout, 0, 1, &state->camera_resources.descriptor_sets[state->image_index], 0, nullptr);
    
    // Draw entities
    for (int i = 0;i < state->entity_count;++i) {
        vkCmdBindDescriptorSets(command_buffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                state->pipeline_layout,
                                1, 1,
                                &state->entity_resources.descriptor_sets[state->image_index],
                                1, &state->entities[i].offset);
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &state->entities[i].buffer, &offset);
        vkCmdDraw(command_buffer, state->entities[i].size, 1, 0, 0);
    }
    
    // Bind the pipeline and the vertex buffer
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->gui_resources.pipeline);
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &state->gui_resources.buffers[state->image_index], &offset);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->gui_resources.pipeline_layout, 0, 1,
                            &state->font_atlas_catalog.resources.descriptor_set, 0, nullptr);
    vkCmdDraw(command_buffer, state->gui_state.current_size, 1, 0, 0);
    vkCmdEndRenderPass(command_buffer);
    vkEndCommandBuffer(command_buffer);
    
    
    VkPipelineStageFlags stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &state->acquire_semaphores[state->current_semaphore_index];
    submit_info.pWaitDstStageMask = &stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &state->present_semaphores[state->current_semaphore_index];
    
    if (state->submissions[state->image_index].command_buffer) {
        vkFreeCommandBuffers(state->device, state->command_pool, 1, &state->submissions[state->image_index].command_buffer);
    }
    
    state->submissions[state->image_index].command_buffer = command_buffer;
    
    result = vkQueueSubmit(state->graphics_queue, 1, &submit_info, *state->submissions[state->image_index].fence);
    if (result != VK_SUCCESS) {
        println("vkQueueSubmit returned (%s)", vk_error_code_str(result));
        return result;
    }
    
    VkResult present_result;
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &state->present_semaphores[state->current_semaphore_index];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &state->swapchain;
    present_info.pImageIndices = &state->image_index;
    present_info.pResults = &present_result;
    
    result = vkQueuePresentKHR(state->present_queue, &present_info);
    if (result != VK_SUCCESS && result != VK_ERROR_OUT_OF_DATE_KHR){
        println("vkQueuePresentKHR returned (%s)", vk_error_code_str(result));
    }
    
    return present_result;
}


inline void do_frame(RendererState* state, WindowUserData* window_user_data, Time* time) {
    update_time(time);
    reset_arena(&state->temporary_storage);
    if (!handle_swapchain_recreation(state, window_user_data)) {
        state->crashed = true;
        return;
    }
    
    VkResult acquire_result = vkAcquireNextImageKHR(state->device, state->swapchain, 0, state->acquire_semaphores[state->current_semaphore_index],
                                                    state->acquire_fences[state->current_semaphore_index],
                                                    &state->image_index);
    if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
        window_user_data->swapchain_need_recreation = true;
        return;
    } else if (acquire_result != VK_SUCCESS) {
        println("vkAcquireNextImageKHR returned (%s)", vk_error_code_str(acquire_result));
        state->crashed = true;
    }
    
    if (!wait_for_acquire_fence(state)) {
        state->crashed = true;
        return;
    }
    if (!wait_for_submit_fence(state)) {
        state->crashed = true;
        return;
    }
    
    
    // Test with fixed timestep
#if 0
    while (time->cumulated_time > 1.0 / 61.0) {
        do_input(state, window_user_data->input);
        update(state, window_user_data->input, time);
        time->cumulated_time -= 1.0 / 59.0;
        if (time->cumulated_time < 0) time->cumulated_time = 0;
    }
#endif
    
    do_input(state, window_user_data->input);
    update(state, window_user_data->input, time);
    
    
    VkResult render_result = render(state);
    if (render_result == VK_ERROR_OUT_OF_DATE_KHR) {
        window_user_data->swapchain_need_recreation = true;
    } else if (render_result != VK_SUCCESS) {
        state->crashed = true;
    }
    
    state->current_semaphore_index = (state->current_semaphore_index + 1) % state->swapchain_image_count;
}

inline void destroy_camera(RendererState* state, bool verbose = true) {
    if (verbose) {
        println("Destroying camera");
    }
    if (state->camera_resources.buffers) {
        for (int i = 0;i < state->swapchain_image_count;i++) {
            if (verbose) {
                println("    Destroying camera data buffer (%p)", state->camera_resources.buffers[i]);
            }
            vkDestroyBuffer(state->device, state->camera_resources.buffers[i], nullptr);
        }
        
        free_null(state->camera_resources.buffers);
    }
    
    if (state->camera_resources.allocations) {
        for (int i = 0;i < state->swapchain_image_count;i++) {
            free(&state->memory_manager, &state->camera_resources.allocations[i]);
        }
        free_null(state->camera_resources.allocations);
    }
    
    if (state->camera_resources.descriptor_sets) {
        free_null(state->camera_resources.descriptor_sets);
    }
    if (verbose) {
        println("");
    }
}

inline void destroy_entity_resources(RendererState* state, bool verbose = true) {
    if (verbose) {
        println("Destroying entity resources");
    }
    if (state->entity_resources.buffers) {
        for (int i = 0;i < state->swapchain_image_count;++i) {
            if (verbose) {
                println("    Destroying entity buffer (%p)", state->entity_resources.buffers[i]);
            }
            vkDestroyBuffer(state->device, state->entity_resources.buffers[i], nullptr);
        }
        
        free_null(state->entity_resources.buffers);
    }
    
    if (state->entity_resources.allocations) {
        for (int i = 0;i < state->swapchain_image_count;++i) {
            free(&state->memory_manager, &state->entity_resources.allocations[i]);
        }
        
        free_null(state->entity_resources.allocations);
    }
    
    if (state->entity_resources.descriptor_sets) {
        free_null(state->entity_resources.descriptor_sets);
    }
    if (verbose) {
        println("");
    }
}

inline void destroy_entities(RendererState* state, bool verbose = true) {
    if (verbose) {
        println("Destroying entities");
    }
    for (int i = 0;i < state->entity_count;++i) {
        if (verbose) {
            println("    Destroying entity %d", i);
        }
        vkDestroyBuffer(state->device, state->entities[i].buffer, nullptr);
        free(&state->memory_manager, &state->entities[i].allocation);
    }
    if (verbose) {
        println("");
    }
}

inline void cleanup(RendererState* state) {
    if (state->submit_fences) {
        for (int i = 0;i < state->swapchain_image_count;++i) {
            vkWaitForFences(state->device, 1, &state->submit_fences[i], VK_TRUE, 1000000000);
        }
    }
    
    u64 start = get_time_ns();
    destroy_window(state, true);
    glfwTerminate();
    
    destroy_entities(state, true);
    destroy_entity_resources(state, true);
    destroy_camera(state, true);
    
    cleanup_gui(&state->gui_state, &state->gui_resources, state, true);
    destroy_material_catalog(&state->material_catalog, state, true);
    destroy_font_atlas_catalog(state, &state->font_atlas_catalog, true);
    cleanup_texture_catalog(state, true);
    destroy_framebuffers(state, true);
    destroy_pipeline(state, true);
    
    destroy_renderpass(state, true);
    destroy_descriptor_set_layout(state, true);
    destroy_descriptor_pool(state, true);
    destroy_pipeline_layout(state, true);
    cleanup_shader_catalog(state->device, &state->shader_catalog, true);
    destroy_command_pool(state, true);
    destroy_fences(state, true);
    destroy_submissions(state, true);
    destroy_semaphores(state, true);
    destroy_depth_images(state, true);
    destroy_swapchain_image_views(state, true);
    destroy_swapchain(state, true);
    cleanup_memory(&state->memory_manager, state->device, true);
    
    destroy_device(state, true);
    destroy_surface(state, true);
    destroy_instance(state, true);
    destroy_memory_arena(&state->temporary_storage, true);
    destroy_memory_arena(&state->main_arena, true);
    u64 end = get_time_ns();
    
    println("Clean up done in %f s", (f64)(end - start) / (f64)(1e9));
}

int main() {
    WindowUserData window_user_data = {};
    Input input = {};
    window_user_data.input = &input;
    
    RendererState state = {};
    
    if (!init(&state, &window_user_data)) {
        cleanup(&state);
        return -1;
    }
    
    FpsCounter fps_counter = {};
    Time time = {};
    time.start = get_time_ns();
    fps_counter.start = get_time_ns();
    
    while (!glfwWindowShouldClose(state.window) && !state.crashed) {
        do_frame(&state, &window_user_data, &time);
        
        update_fps_counter(&state, state.window, &fps_counter);
    }
    
    cleanup(&state);
    return state.crashed ? -1 : 0;
}
