#include "cg_gui.h"

#include <assert.h>

#include "cg_macros.h"
#include "cg_renderer.h"
#include "cg_vk_helper.h"
#include "cg_memory.h"

inline bool create_gui_descriptor_set_layout(GuiResources* resources, RendererState* state) {
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = 1;
    create_info.pBindings = &binding;
    
    VkResult result = vkCreateDescriptorSetLayout(state->device, &create_info, nullptr, &resources->descriptor_set_layout);
    
    if (result != VK_SUCCESS) {
        println("vkCreateDescriptorSetLayout returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline bool create_gui_pipeline_layout(GuiResources* resources, RendererState* state) {
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &resources->descriptor_set_layout;
    
    VkResult result = vkCreatePipelineLayout(state->device, &pipeline_layout_create_info, nullptr, &resources->pipeline_layout);
    if (result != VK_SUCCESS) {
        println("vkCreatePipelineLayout returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline bool create_gui_pipeline(GuiResources* resources, RendererState* state) {
    VkPipelineShaderStageCreateInfo stage_create_info[2] = {};
    stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    if (!get_shader_from_catalog("gui.vert", &state->shader_catalog, &stage_create_info[0].module)) {
        return false;
    }
    stage_create_info[0].pName = "main";
    
    stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    if (!get_shader_from_catalog("gui.frag", &state->shader_catalog, &stage_create_info[1].module)) {
        return false;
    }
    stage_create_info[1].pName = "main";
    
    VkVertexInputBindingDescription binding_description = {};
    binding_description.binding = 0;
    binding_description.stride = sizeof(GuiVertex);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription attribute_description[5] = {};
    attribute_description[0].location = 0;
    attribute_description[0].binding = 0;
    attribute_description[0].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_description[0].offset = offsetof(GuiVertex, position);
    
    attribute_description[1].location = 1;
    attribute_description[1].binding = 0;
    attribute_description[1].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_description[1].offset = offsetof(GuiVertex, uv);
    
    attribute_description[2].location = 2;
    attribute_description[2].binding = 0;
    attribute_description[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attribute_description[2].offset = offsetof(GuiVertex, color);
    
    attribute_description[3].location = 3;
    attribute_description[3].binding = 0;
    attribute_description[3].format = VK_FORMAT_R32_SFLOAT;
    attribute_description[3].offset = offsetof(GuiVertex, text_blend);
    
    attribute_description[4].location = 4;
    attribute_description[4].binding = 0;
    attribute_description[4].format = VK_FORMAT_R32_UINT;
    attribute_description[4].offset = offsetof(GuiVertex, font_index);
    
    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = array_size(attribute_description);
    vertex_input_state_create_info.pVertexAttributeDescriptions = attribute_description;
    
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
    input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width  = (f32)state->swapchain_extent.width;
    viewport.height = (f32)state->swapchain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = state->swapchain_extent;
    
    VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.depthClampEnable = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    rasterization_state_create_info.lineWidth = 1.0f;
    
    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
    multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    
    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    
    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.logicOpEnable = VK_FALSE;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment;
    
    VkGraphicsPipelineCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.stageCount = 2;
    create_info.pStages = stage_create_info;
    create_info.pVertexInputState = &vertex_input_state_create_info;
    create_info.pInputAssemblyState = &input_assembly_state_create_info;
    create_info.pViewportState = &viewport_state_create_info;
    create_info.pRasterizationState = &rasterization_state_create_info;
    create_info.pMultisampleState = &multisample_state_create_info;
    create_info.pDepthStencilState = &depth_stencil_state_create_info;
    create_info.pColorBlendState = &color_blend_state_create_info;
    create_info.layout = resources->pipeline_layout;
    create_info.renderPass = state->renderpass;
    create_info.subpass = 0;
    
    VkResult result = vkCreateGraphicsPipelines(state->device, VK_NULL_HANDLE, 1, &create_info, nullptr, &resources->pipeline);
    if (result != VK_SUCCESS) {
        println("vkCreateGraphicsPipelines returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline bool create_gui_buffers(GuiResources* resources, RendererState* state, MemoryArena* storage) {
    resources->buffers = (VkBuffer*)zero_allocate(storage, state->swapchain_image_count * sizeof(VkBuffer));
    resources->allocations = (AllocatedMemoryChunk*)zero_allocate(storage, state->swapchain_image_count * sizeof(AllocatedMemoryChunk));
    
    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size  = MAX_GUI_VERTEX_COUNT * sizeof(GuiVertex);
    create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &state->selection.graphics_queue_family_index;
    
    for (int i = 0;i < state->swapchain_image_count;++i) {
        VkResult result = vkCreateBuffer(state->device, &create_info, nullptr, &resources->buffers[i]);
        if (result != VK_SUCCESS) {
            println("vkCreateBuffer returned (%s)", vk_error_code_str(result));
            return false;
        }
        
        VkMemoryRequirements requirements = {};
        
        vkGetBufferMemoryRequirements(state->device, resources->buffers[i], &requirements);
        
        VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        
        if(!allocate(&state->memory_manager, state->device, requirements, memory_flags, &resources->allocations[i])) {
            return false;
        }
        
        result = vkBindBufferMemory(state->device, resources->buffers[i], resources->allocations[i].device_memory, resources->allocations[i].offset);
        if (result != VK_SUCCESS) {
            println("vkBindBufferMemory returned (%s)", vk_error_code_str(result));
            return false;
        }
    }
    
    return true;
}

inline bool init_gui_resources(GuiResources* resources, RendererState* state) {
    resources->font_atlas_slot_count = 64;
    if (!create_gui_descriptor_set_layout(resources, state)) {
        println("Error: failed to create gui descriptor set layout");
        return false;
    }
    
    if (!create_gui_pipeline_layout(resources, state)) {
        println("Error: failed to create gui pipeline layout");
        return false;
    }
    
    if (!create_gui_pipeline(resources, state)) {
        println("Error: failed to create gui pipeline");
        return false;
    }
    
    if (!create_gui_buffers(resources, state, &state->main_arena)) {
        println("Error: failed to create gui buffers");
        return false;
    }
    
    if (!init_memory_arena(&resources->main_arena, MB(1))) {
        println("Error: failed to initialize gui memory arena");
        return false;
    }
    
    return true;
}

inline void destroy_gui_buffers(GuiResources* resources, RendererState* state, bool verbose) {
    if (verbose) {
        println("Destroying GUI buffers");
    }
    if (resources->buffers) {
        for(int i = 0;i < state->swapchain_image_count;++i) {
            if (verbose) {
                println("    Destroying GUI buffer (%p)", resources->buffers[i]);
            }
            vkDestroyBuffer(state->device, resources->buffers[i], nullptr);
        }
    }
    
    if (resources->allocations) {
        for(int i = 0;i < state->swapchain_image_count;++i) {
            free(&state->memory_manager, &resources->allocations[i]);
        }
    }
    if (verbose) {
        println("");
    }
}

inline void destroy_gui_pipeline(GuiResources* resources, RendererState* state, bool verbose) {
    if (resources->pipeline) {
        if (verbose) {
            println("Destroying gui pipeline (%p)", resources->pipeline);
        }
        vkDestroyPipeline(state->device, resources->pipeline, nullptr);
        resources->pipeline= 0;
    }
    if (verbose) {
        println("");
    }
}

inline void destroy_gui_pipeline_layout(GuiResources* resources, RendererState* state, bool verbose) {
    if (resources->pipeline_layout) {
        if (verbose) {
            println("Destroying gui pipeline layout (%p)", resources->pipeline_layout);
        }
        vkDestroyPipelineLayout(state->device, resources->pipeline_layout, nullptr);
        resources->pipeline_layout = 0;
    }
    if (verbose) {
        println("");
    }
}

inline void destroy_gui_descriptor_set_layout(GuiResources* resources, RendererState* state, bool verbose) {
    if (resources->descriptor_set_layout) {
        if (verbose) {
            println("Destroying gui decriptor set layout (%p)", resources->descriptor_set_layout);
        }
        vkDestroyDescriptorSetLayout(state->device, resources->descriptor_set_layout, nullptr);
        resources->descriptor_set_layout = 0;
    }
    if (verbose) {
        println("");
    }
}


inline bool destroy_gui_resources(GuiResources* resources, RendererState* state, bool verbose) {
    destroy_memory_arena(&resources->main_arena);
    destroy_gui_buffers(resources, state, verbose);
    destroy_gui_pipeline(resources, state, verbose);
    destroy_gui_pipeline_layout(resources, state, verbose);
    destroy_gui_descriptor_set_layout(resources, state, verbose);
}

inline bool init_gui_state(GuiState* gui_state, GuiResources* resources, RendererState* renderer_state, MemoryArena* storage) {
    gui_state->vertex_buffer = (GuiVertex*)zero_allocate(storage, MAX_GUI_VERTEX_COUNT * sizeof(GuiVertex));
    gui_state->current_size = 0;
    gui_state->screen_size.x = (i32)renderer_state->swapchain_extent.width;
    gui_state->screen_size.y = (i32)renderer_state->swapchain_extent.height;
    gui_state->window = renderer_state->window;
    
    gui_state->resources = resources;
    return true;
}

inline bool init_gui(GuiState* gui_state, GuiResources* resources, RendererState* renderer_state) {
    if (!init_gui_resources(resources, renderer_state)) {
        println("Failed to initialize gui resources");
        return false;
    }
    
    if (!init_gui_state(gui_state, resources, renderer_state, &renderer_state->main_arena)) {
        println("Failed to initialize gui state");
        return false;
    }
    
    return true;
}

inline void reset_gui(GuiState* state, GuiResources* resources) {
    state->current_size = 0;
    reset_arena(&resources->main_arena);
}

inline void cleanup_gui(GuiState* gui_state, GuiResources* resources, RendererState* renderer_state, bool verbose) {
    destroy_gui_resources(resources, renderer_state, verbose);
}

inline Vec2f screen_space_to_normalized_space(Vec2u screen_size, i32 x, i32 y) {
    Vec2f result = {};
    
    result.x = ((f32)x / (f32)screen_size.x) * 2.0f - 1.0f;
    result.y = ((f32)y / (f32)screen_size.y) * 2.0f - 1.0f;
    
    return result;
}

inline Vec2f screen_space_to_normalized_space(Vec2u screen_size, Vec2i input) {
    return screen_space_to_normalized_space(screen_size, input.x, input.y);
}

inline void draw_rectangle(GuiState* state, i32 left, i32 top, i32 right, i32 bottom, Vec4f color) {
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

inline bool draw_button(GuiState* state, Input* input, bool button_state, i32 left, i32 top, i32 right, i32 bottom, Vec4f color, Vec4f hover_color, Vec4f active_color) {
    f64 mouse_x, mouse_y;
    
    glfwGetCursorPos(state->window, &mouse_x, &mouse_y);
    // NOTE: what is the behaviour of this comparison ?
    bool inside = mouse_x >= left &&
        mouse_x <= right &&
        mouse_y >= top &&
        mouse_y <= bottom;
    
    if (button_state || (inside && input->button_pressed[GLFW_MOUSE_BUTTON_LEFT])) {
        draw_rectangle(state, left, top, right, bottom, active_color);
    } else if (inside) {
        draw_rectangle(state, left, top, right, bottom, hover_color);
    } else {
        draw_rectangle(state, left, top, right, bottom, color);
    }
    return (inside && input->button_pressed[GLFW_MOUSE_BUTTON_LEFT]);
}

inline Rect2i get_text_bounding_box(ConstString* text, FontAtlas* font_atlas) {
    Rect2i bounding_box = {};
    i32* left   = &bounding_box.left;
    i32* top    = &bounding_box.top;
    i32* right  = &bounding_box.right;
    i32* bottom = &bounding_box.bottom;
    
    i32 x_pos = 0;
    for (u32 i = 0;i < text->size;++i) {
        char* c = text->str + i;
        u32 index = get_ascii_character_index(font_atlas, *c);
        Glyph2* g = font_atlas->glyphs + index;
        
        *left   = min(*left, x_pos + g->x_offset);
        *right  = max(*right, x_pos + max(g->x_offset_2, g->x_advance));
        *top    = min(*top, g->y_offset);
        *bottom = max(*bottom, g->y_offset_2);
        
        x_pos += g->x_advance;
    }
    
    return bounding_box;
}

inline Vec2u get_text_dimensions(ConstString* text, FontAtlas* font_atlas) {
    Rect2i bounding_box = get_text_bounding_box(text, font_atlas);
    return new_vec2u(bounding_box.right - bounding_box.left, bounding_box.bottom - bounding_box.top);
}

inline GuiVertex make_text_vertex(GuiState* state, FontAtlas* font_atlas, i32 x, i32 y, u32 u, u32 v, u32 font_index, Vec4f color) {
    GuiVertex vertex = {};
    
    vertex.position_int   = {x, y};
    vertex.uv         = new_vec2f((f32)u / (f32)font_atlas->width, (f32)v / (f32)font_atlas->height);
    vertex.text_blend = 1.0f;
    vertex.color      = color;
    vertex.font_index = font_index;
    
    return vertex;
}

inline GuiVertex make_text_vertex(GuiState* state, FontAtlas* font_atlas, Vec2i position, Vec2u uv, u32 font_index, Vec4f color) {
    return make_text_vertex(state, font_atlas, position.x, position.y, uv.x, uv.y, font_index, color);
}

inline Vec2i compute_offset_from_bounding_box(Rect2i bounding_box, i32 x, i32 y, TextAnchor text_anchor) {
    Vec2i offset = {x, y};
    
    i32 height = 0;
    i32 width  = 0;
    switch (text_anchor) {
        case TopLeft:
        offset.x += -bounding_box.left;
        offset.y += -bounding_box.top;
        break;
        
        case BottomLeft:
        offset.x += -bounding_box.left;
        offset.y += -bounding_box.bottom;
        break;
        
        case TopRight:
        offset.x += -bounding_box.right;
        offset.y += -bounding_box.top;
        break;
        
        case BottomRight:
        offset.x += -bounding_box.right;
        offset.y += -bounding_box.bottom;
        break;
        
        case MiddleLeft:
        height = bounding_box.bottom - bounding_box.top;
        offset.x += -bounding_box.left;
        offset.y += -bounding_box.top - height / 2;
        break;
        
        case MiddleRight:
        height = bounding_box.bottom - bounding_box.top;
        offset.x += -bounding_box.right;
        offset.y += -bounding_box.top - height / 2;
        break;
        
        case TopMiddle:
        width = bounding_box.right - bounding_box.left;
        offset.x += -bounding_box.left - width / 2;
        offset.y += -bounding_box.top;
        break;
        
        case BottomMiddle:
        width = bounding_box.right - bounding_box.left;
        offset.x += -bounding_box.left - width / 2;
        offset.y += -bounding_box.bottom;
        break;
        
        case Center:
        width =  bounding_box.right - bounding_box.left;
        height = bounding_box.bottom - bounding_box.top;
        offset.x += -bounding_box.left - width / 2;
        offset.y += -bounding_box.top - height / 2;
        break;
    }
    return offset;
}

inline void draw_text(GuiState* state, ConstString* text,
                      i32 x, i32 y, Vec4f color,
                      TextAnchor text_anchor,
                      FontAtlas* font_atlas) {
    assert((state->current_size + 6 * text->size) < MAX_GUI_VERTEX_COUNT);
    
    u32 start_index = state->current_size;
    
    i32 pos_x = 0;
    i32 pos_y = 0;
    
    Rect2i bounding_box = {};
    i32* left   = &bounding_box.left;
    i32* top    = &bounding_box.top;
    i32* right  = &bounding_box.right;
    i32* bottom = &bounding_box.bottom;
    for (u32 i = 0;i < text->size;++i) {
        char* c = text->str + i;
        u32 index = get_ascii_character_index(font_atlas, *c);
        Glyph2* g = font_atlas->glyphs + index;
        
        // Compute quad
        i32 text_left   = pos_x + g->x_offset;
        i32 text_right  = pos_x + g->x_offset_2;
        i32 text_top    = pos_y + g->y_offset;
        i32 text_bottom = pos_y + g->y_offset_2;
        
        GuiVertex bottom_left_vertex = make_text_vertex(state, font_atlas,
                                                        text_left, text_bottom,
                                                        g->left, g->bottom,
                                                        font_atlas->texture_array_index,
                                                        color);
        
        GuiVertex top_left_vertex = make_text_vertex(state, font_atlas,
                                                     text_left, text_top,
                                                     g->left, g->top,
                                                     font_atlas->texture_array_index,
                                                     color);
        
        GuiVertex bottom_right_vertex = make_text_vertex(state, font_atlas,
                                                         text_right, text_bottom,
                                                         g->right, g->bottom,
                                                         font_atlas->texture_array_index,
                                                         color);
        
        GuiVertex top_right_vertex = make_text_vertex(state, font_atlas,
                                                      text_right, text_top,
                                                      g->right, g->top,
                                                      font_atlas->texture_array_index,
                                                      color);
        
        // Append quad to vertex buffer
        state->vertex_buffer[state->current_size++] = bottom_left_vertex;
        state->vertex_buffer[state->current_size++] = top_left_vertex;
        state->vertex_buffer[state->current_size++] = top_right_vertex;
        
        state->vertex_buffer[state->current_size++] = bottom_left_vertex;
        state->vertex_buffer[state->current_size++] = top_right_vertex;
        state->vertex_buffer[state->current_size++] = bottom_right_vertex;
        
        // Update bounding box
        *left   = min(*left, pos_x + g->x_offset);
        *right  = max(*right, pos_x + max(g->x_offset_2, g->x_advance));
        *top    = min(*top, g->y_offset);
        *bottom = max(*bottom, g->y_offset_2);
        
        // Update current x_pos
        pos_x += g->x_advance;
    }
    
    Vec2i offset = compute_offset_from_bounding_box(bounding_box, x, y, text_anchor);
    
    for (u32 i = start_index;i < state->current_size;++i) {
        GuiVertex* vertex = state->vertex_buffer + i;
        vertex->position_int.x += offset.x;
        vertex->position_int.y += offset.y;
        
        vertex->position = screen_space_to_normalized_space(state->screen_size, vertex->position_int);
    }
}

inline char* to_string(GuiState* state, MemoryArena* temporary_storage, u32 indentation_level) {
    char* indent_space = (char*)allocate(temporary_storage, indentation_level + 1);
    for (u32 i = 0;i < indentation_level;i++) {
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