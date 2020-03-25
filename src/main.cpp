#include <iostream>
#include <cstdlib>
#include <string>
#include <thread>
#include <cstring>
#include <cmath>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "macros.hpp"
#include "input.hpp"
#include "window.hpp"
#include "window_user_data.hpp"
#include "renderer.hpp"
#include "shaders.hpp"
#include "vk_helper.hpp"

struct FpsCounter {
    std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds> start;
    std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds> end;
    uint64_t cumulated_frame_duration;
    uint32_t frame_count;
};

const char* get_device_type_str(VkPhysicalDeviceType type) {
    switch (type) {
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
	return "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU";
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
	return "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU";
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
	return "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU";
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
	return "VK_PHYSICAL_DEVICE_TYPE_CPU";
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
    default:
	return "VK_PHYSICAL_DEVICE_TYPE_OTHER";
    }
}

struct Time {
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::chrono::time_point<std::chrono::high_resolution_clock> end;
    float delta_time;
    float cumulated_time;
};

void do_input(Input* input) {
    reset_input(input);
    glfwPollEvents();
}

void update_fps_counter(GLFWwindow* window, FpsCounter* fps_counter) {
    fps_counter->end = std::chrono::high_resolution_clock::now();
    auto last_start = fps_counter->start;
    fps_counter->start = std::chrono::high_resolution_clock::now();

    fps_counter->cumulated_frame_duration += (fps_counter->end - last_start).count();
    fps_counter->frame_count++;

    if (fps_counter->frame_count == 1000) {
	double average_frame_duration = (double)fps_counter->cumulated_frame_duration / (double)fps_counter->frame_count;
	average_frame_duration /= 1000000000.0;

	fps_counter->frame_count = 0;
	fps_counter->cumulated_frame_duration = 0;

	std::string avg_frame_duration_str = std::to_string(1.0 / average_frame_duration) + " fps";
	glfwSetWindowTitle(window, avg_frame_duration_str.data());
    }
}


bool handle_swapchain_recreation(RendererState* state, WindowUserData* window_user_data) {
    if (window_user_data->swapchain_need_recreation) {
	// Wait for the last frame in flight to be ready
	VkResult result = vkQueueWaitIdle(state->graphics_queue);
	if (result != VK_SUCCESS) {
	    std::cout << "Failed to wait for queue idle" << std::endl;
	    return false;
	}

	result = vkQueueWaitIdle(state->present_queue);
	if (result != VK_SUCCESS) {
	    std::cout << "Failed to wait for queue idle" << std::endl;
	    return false;
	}

	// Destroy the pipeline
	destroy_framebuffers(state);
	destroy_pipeline(state);
	destroy_depth_images(state);

	if (!create_swapchain(state)) {
	    std::cout << "Failed to recreate swapchain" << std::endl;
	    return false;
	}

	free_null(state->swapchain_images);
	if (!get_swapchain_images(state)) {
	    std::cout << "Failed to get swapchain images" << std::endl;
	    return false;
	}

	destroy_swapchain_image_views(state);
	if (!create_swapchain_image_views(state)) {
	    std::cout << "Failed to create swapchain image views" << std::endl;
	    return false;
	}

	if (!create_depth_images(state)) {
	    std::cout << "Failed to create depth image" << std::endl;
	    return false;
	}

	if(!create_graphics_pipeline(state)) {
	    std::cout << "Failed to recreate pipeline" << std::endl;
	}

	if(!create_framebuffers(state)) {
	    std::cout << "Failed to recreate framebuffers" << std::endl;
	}

	window_user_data->swapchain_need_recreation = false;

	state->camera.aspect = (float)state->swapchain_extent.width / (float)state->swapchain_extent.height;
	state->camera_data.context.projection = perspective(state->camera.fov, state->camera.aspect, 0.1f, 100.0f);
    }

    return true;
}

bool create_square_entity(RendererState* state) {
    float z = 0.0f;
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

    uint32_t buffer_size = array_size(vertex_buffer) * sizeof(Vertex);

    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = buffer_size;
    create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &state->selection.graphics_queue_family_index;

    VkResult result = vkCreateBuffer(state->device, &create_info, nullptr, &state->square_entity.buffer);
    if (result != VK_SUCCESS) {
	return false;
    }

    VkMemoryRequirements requirements = {};

    vkGetBufferMemoryRequirements(state->device, state->square_entity.buffer, &requirements);

    VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    if(!allocate(&state->memory_manager, state->device, requirements, memory_flags, &state->square_entity.allocation)) {
	return false;
    }

    result = vkBindBufferMemory(state->device, state->square_entity.buffer, state->square_entity.allocation.device_memory, state->square_entity.allocation.offset);
    if (result != VK_SUCCESS) {
	return false;
    }

    memcpy(state->square_entity.allocation.data, vertex_buffer, buffer_size);

    return true;
}

void create_cube(Vec3f size, Vec3f position, Vertex* vertices) {
    // Front face
    vertices[ 0].position = new_vec3f(-size.x + position.x, -size.y + position.y,  size.z + position.z);
    vertices[ 1].position = new_vec3f(-size.x + position.x,  size.y + position.y,  size.z + position.z);
    vertices[ 2].position = new_vec3f( size.x + position.x, -size.y + position.y,  size.z + position.z);

    vertices[ 3].position = new_vec3f( size.x + position.x, -size.y + position.y,  size.z + position.z);
    vertices[ 4].position = new_vec3f(-size.x + position.x,  size.y + position.y,  size.z + position.z);
    vertices[ 5].position = new_vec3f( size.x + position.x,  size.y + position.y,  size.z + position.z);

    vertices[ 0].color = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[ 1].color = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[ 2].color = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[ 3].color = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[ 4].color = new_vec3f(1.0f, 0.0f, 0.0f);
    vertices[ 5].color = new_vec3f(1.0f, 0.0f, 0.0f);

    // Right face
    vertices[ 6].position = new_vec3f( size.x + position.x, -size.y + position.y,  size.z + position.z);
    vertices[ 7].position = new_vec3f( size.x + position.x,  size.y + position.y,  size.z + position.z);
    vertices[ 8].position = new_vec3f( size.x + position.x, -size.y + position.y, -size.z + position.z);

    vertices[ 9].position = new_vec3f( size.x + position.x, -size.y + position.y, -size.z + position.z);
    vertices[10].position = new_vec3f( size.x + position.x,  size.y + position.y,  size.z + position.z);
    vertices[11].position = new_vec3f( size.x + position.x,  size.y + position.y, -size.z + position.z);

    vertices[ 6].color = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[ 7].color = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[ 8].color = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[ 9].color = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[10].color = new_vec3f(0.0f, 1.0f, 0.0f);
    vertices[11].color = new_vec3f(0.0f, 1.0f, 0.0f);

    // Back face
    vertices[12].position = new_vec3f( size.x + position.x, -size.y + position.y, -size.z + position.z);
    vertices[13].position = new_vec3f( size.x + position.x,  size.y + position.y, -size.z + position.z);
    vertices[14].position = new_vec3f(-size.x + position.x, -size.y + position.y, -size.z + position.z);

    vertices[15].position = new_vec3f(-size.x + position.x, -size.y + position.y, -size.z + position.z);
    vertices[16].position = new_vec3f( size.x + position.x,  size.y + position.y, -size.z + position.z);
    vertices[17].position = new_vec3f(-size.x + position.x,  size.y + position.y, -size.z + position.z);

    vertices[12].color = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[13].color = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[14].color = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[15].color = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[16].color = new_vec3f(0.0f, 0.0f, 1.0f);
    vertices[17].color = new_vec3f(0.0f, 0.0f, 1.0f);

    // Left face
    vertices[18].position = new_vec3f(-size.x + position.x, -size.y + position.y, -size.z + position.z);
    vertices[19].position = new_vec3f(-size.x + position.x,  size.y + position.y, -size.z + position.z);
    vertices[20].position = new_vec3f(-size.x + position.x, -size.y + position.y,  size.z + position.z);

    vertices[21].position = new_vec3f(-size.x + position.x, -size.y + position.y,  size.z + position.z);
    vertices[22].position = new_vec3f(-size.x + position.x,  size.y + position.y, -size.z + position.z);
    vertices[23].position = new_vec3f(-size.x + position.x,  size.y + position.y,  size.z + position.z);

    vertices[18].color = new_vec3f(1.0f, 0.0f, 1.0f);
    vertices[19].color = new_vec3f(1.0f, 0.0f, 1.0f);
    vertices[20].color = new_vec3f(1.0f, 0.0f, 1.0f);
    vertices[21].color = new_vec3f(1.0f, 0.0f, 1.0f);
    vertices[22].color = new_vec3f(1.0f, 0.0f, 1.0f);
    vertices[23].color = new_vec3f(1.0f, 0.0f, 1.0f);

    // Top face
    vertices[24].position = new_vec3f( size.x + position.x,  size.y + position.y,  size.z + position.z);
    vertices[25].position = new_vec3f(-size.x + position.x,  size.y + position.y,  size.z + position.z);
    vertices[26].position = new_vec3f( size.x + position.x,  size.y + position.y, -size.z + position.z);

    vertices[27].position = new_vec3f( size.x + position.x,  size.y + position.y, -size.z + position.z);
    vertices[28].position = new_vec3f(-size.x + position.x,  size.y + position.y,  size.z + position.z);
    vertices[29].position = new_vec3f(-size.x + position.x,  size.y + position.y, -size.z + position.z);

    vertices[24].color = new_vec3f(1.0f, 1.0f, 0.0f);
    vertices[25].color = new_vec3f(1.0f, 1.0f, 0.0f);
    vertices[26].color = new_vec3f(1.0f, 1.0f, 0.0f);
    vertices[27].color = new_vec3f(1.0f, 1.0f, 0.0f);
    vertices[28].color = new_vec3f(1.0f, 1.0f, 0.0f);
    vertices[29].color = new_vec3f(1.0f, 1.0f, 0.0f);

    // Bottom face
    vertices[30].position = new_vec3f(-size.x + position.x, -size.y + position.y,  size.z + position.z);
    vertices[31].position = new_vec3f( size.x + position.x, -size.y + position.y,  size.z + position.z);
    vertices[32].position = new_vec3f( size.x + position.x, -size.y + position.y, -size.z + position.z);

    vertices[33].position = new_vec3f( size.x + position.x, -size.y + position.y, -size.z + position.z);
    vertices[34].position = new_vec3f(-size.x + position.x, -size.y + position.y, -size.z + position.z);
    vertices[35].position = new_vec3f(-size.x + position.x, -size.y + position.y,  size.z + position.z);

    vertices[30].color = new_vec3f(0.0f, 1.0f, 1.0f);
    vertices[31].color = new_vec3f(0.0f, 1.0f, 1.0f);
    vertices[32].color = new_vec3f(0.0f, 1.0f, 1.0f);
    vertices[33].color = new_vec3f(0.0f, 1.0f, 1.0f);
    vertices[34].color = new_vec3f(0.0f, 1.0f, 1.0f);
    vertices[35].color = new_vec3f(0.0f, 1.0f, 1.0f);
}

bool create_cube_entity(RendererState* state) {
    Vertex vertex_buffer[36] = {};
    create_cube(new_vec3f(0.2f, 0.2f, 0.2f), new_vec3f(1.0f, 0.0f, 0.0f), vertex_buffer);

    uint32_t buffer_size = array_size(vertex_buffer) * sizeof(Vertex);

    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = buffer_size;
    create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &state->selection.graphics_queue_family_index;

    VkResult result = vkCreateBuffer(state->device, &create_info, nullptr, &state->cube_entity.buffer);
    if (result != VK_SUCCESS) {
	return false;
    }

    VkMemoryRequirements requirements = {};

    vkGetBufferMemoryRequirements(state->device, state->cube_entity.buffer, &requirements);

    VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    if(!allocate(&state->memory_manager, state->device, requirements, memory_flags, &state->cube_entity.allocation)) {
	return false;
    }

    result = vkBindBufferMemory(state->device, state->cube_entity.buffer, state->cube_entity.allocation.device_memory, state->cube_entity.allocation.offset);
    if (result != VK_SUCCESS) {
	return false;
    }

    memcpy(state->cube_entity.allocation.data, vertex_buffer, buffer_size);

    return true;
}

bool allocate_camera_descriptor_sets(RendererState* state) {
    state->camera_data.descriptor_sets = (VkDescriptorSet*)calloc(state->swapchain_image_count, sizeof(VkDescriptorSet));

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

    VkResult result = vkAllocateDescriptorSets(state->device, &allocate_info, state->camera_data.descriptor_sets);
    if (result != VK_SUCCESS) {
	return false;
    }

    free_null(layouts);

    return true;
}

bool create_camera_buffers(RendererState* state) {
    state->camera_data.buffers     = (VkBuffer*)            calloc(state->swapchain_image_count, sizeof(VkBuffer));
    state->camera_data.allocations = (AllocatedMemoryChunk*)calloc(state->swapchain_image_count, sizeof(AllocatedMemoryChunk));

    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = sizeof(Context);
    create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &state->selection.graphics_queue_family_index;

    for (int i = 0;i < state->swapchain_image_count;++i) {
	VkResult result = vkCreateBuffer(state->device, &create_info, nullptr, &state->camera_data.buffers[i]);
	if (result != VK_SUCCESS) {
	    return false;
	}

	VkMemoryRequirements requirements = {};

	vkGetBufferMemoryRequirements(state->device, state->camera_data.buffers[i], &requirements);

	VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	if(!allocate(&state->memory_manager, state->device, requirements, memory_flags, &state->camera_data.allocations[i])) {
	    return false;
	}

	result = vkBindBufferMemory(state->device, state->camera_data.buffers[i], state->camera_data.allocations[i].device_memory, state->camera_data.allocations[i].offset);
	if (result != VK_SUCCESS) {
	    return false;
	}

	memcpy(state->camera_data.allocations[i].data, &state->camera_data.context, sizeof(Context));
    }

    return true;
}

void update_camera_descriptor_set(RendererState* state) {
    VkDescriptorBufferInfo* buffer_info;

    buffer_info = (VkDescriptorBufferInfo*)calloc(state->swapchain_image_count, sizeof(VkDescriptorBufferInfo));

    for (int i = 0;i < state->swapchain_image_count;++i) {
	buffer_info[i].buffer = state->camera_data.buffers[i];
	buffer_info[i].offset = 0;
	buffer_info[i].range = sizeof(Context);
    }

    VkWriteDescriptorSet* writes;

    writes = (VkWriteDescriptorSet*)calloc(state->swapchain_image_count, sizeof(VkWriteDescriptorSet));

    for (int i = 0;i < state->swapchain_image_count;++i) {
	writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[i].dstSet = state->camera_data.descriptor_sets[i];
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

bool init_camera(RendererState* state) {
    state->camera.fov = 70.0f;
    state->camera.position = new_vec3f(0.0f, 0.0f, 2.0f);
    state->camera.center = new_vec3f(0.0f, 0.0f, 0.0f);
    state->camera.aspect = (float)state->swapchain_extent.width / (float)state->swapchain_extent.height;

    state->camera_data.context.projection = perspective(state->camera.fov, state->camera.aspect, 0.1f, 100.0f);
    state->camera_data.context.view = look_at(state->camera.position, state->camera.center, new_vec3f(0.0f, 1.0f, 0.0f));

    if (!allocate_camera_descriptor_sets(state)) {
	return false;
    }

    if (!create_camera_buffers(state)) {
	return false;
    }

    update_camera_descriptor_set(state);

    return true;
}

bool init(RendererState* state, WindowUserData* window_user_data) {
    if (!glfwInit()) {
	std::cout << "Error: failed to initialize glfw library" << std::endl;
	return false;
    } else {
	std::cout << "glfw init: success" << std::endl;
    }

    if (!glfwVulkanSupported()) {
	std::cout << "Vulkan is not supported" << std::endl;
	return false;
    } else {
	std::cout << "vulkan: supported" << std::endl;
    }

    if (!create_instance(&state->instance)) {
	return false;
    } else {
	std::cout << "instance init: success" << std::endl;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    state->window = glfwCreateWindow(1280, 720, "vkRenderer", nullptr, nullptr);

    if (!state->window) {
	std::cout << "Error: failed to create window" << std::endl;
	return false;
    } else {
	std::cout << "window init: success" << std::endl;
    }

    VkResult result = glfwCreateWindowSurface(state->instance, state->window, nullptr, &state->surface);
    if(result != VK_SUCCESS) {
	std::cout << "Error: failed to create window surface (" << result << ")" << std::endl;
	return false;
    } else {
	std::cout << "surface init: success" << std::endl;
    }

    uint32_t extension_count = 0;
    const char** required_instance_extensions = glfwGetRequiredInstanceExtensions(&extension_count);

    if (!check_required_instance_extensions(required_instance_extensions, extension_count)) {
	return false;
    } else {
	std::cout << "required extensions: supported" << std::endl;
    }

    if (!select_physical_device(state)) {
	std::cout << "Error: failed to select a physical device." << std::endl;
	return false;
    } else {
	std::cout << "physical device selection: success" << std::endl;
    }

    if (!check_required_device_extensions(state->selection.device)) {
	return false;
    } else {
	std::cout << "required device extensions: supported" << std::endl;
    }

    if (!create_device_and_queues(state)) {
	std::cout << "Error: failed to create the device." << std::endl;
	return false;
    } else {
	std::cout << "device and queues init: success" << std::endl;
    }

    uint32_t allocation_size = mega(128);
    uint32_t min_page_size = kilo(4);

    if (!init_memory(&state->memory_manager, allocation_size, min_page_size, state->selection.device)) {
	return false;
    } else {
	std::cout << "memory manager init: success" << std::endl;
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
	std::cout << "swapchain init: success" << std::endl;
    }

    if (!get_swapchain_images(state)) {
	return false;
    } else {
	std::cout << "swapchain images init: success" << std::endl;
    }

    if (!create_depth_images(state)) {
	return false;
    } else {
	std::cout << "depth images init: success" << std::endl;
    }

    if (!create_semaphore(state)) {
	return false;
    }

    if (!create_fence(state)) {
	return false;
    }

    if (!create_command_pool(state)) {
	return false;
    } else {
	std::cout << "command pool init: success" << std::endl;
    }

    if (!create_swapchain_image_views(state)) {
	return false;
    } else {
	std::cout << "swapchain image view init: success" << std::endl;
    }

    if (!init_shader_catalog(64, &state->shader_catalog)) {
	return false;
    } else {
	std::cout << "shader catalog init: success" << std::endl;
    }

    if (!load_shader_module("resources/shaders/basic.vert.spv", "basic.vert", state->device, &state->shader_catalog)) {
	return false;
    } else {
	std::cout << "basic vertex shader init: success" << std::endl;
    }

    if (!load_shader_module("resources/shaders/basic.frag.spv", "basic.frag", state->device, &state->shader_catalog)) {
	return false;
    } else {
	std::cout << "basic fragment shader init: success" << std::endl;
    }

    if (!create_descriptor_set_layout(state)) {
	return false;
    } else {
	std::cout << "descriptor set layout init: success" << std::endl;
    }

    if (!create_descriptor_pool(state)) {
	return false;
    } else {
	std::cout << "descriptor pool init: success" << std::endl;
    }

    if (!create_pipeline_layout(state)) {
	return false;
    } else {
	std::cout << "pipeline layout init: success" << std::endl;
    }

    if (!create_render_pass(state)) {
	return false;
    } else {
	std::cout << "render pass init: success" << std::endl;
    }

    if (!create_graphics_pipeline(state)) {
	return false;
    } else {
	std::cout << "graphics pipeline init: success" << std::endl;
    }

    if (!create_framebuffers(state)) {
	return false;
    } else {
	std::cout << "framebuffers init: success" << std::endl;
    }

    if (!create_square_entity(state)) {
	return false;
    } else {
	std::cout << "square entity init : success" << std::endl;
    }

    if (!create_cube_entity(state)) {
	return false;
    } else {
	std::cout << "cube entity init : success" << std::endl;
    }

    if (!init_camera(state)) {
	return false;
    } else {
	std::cout << "camera init : success" << std::endl;
    }

    glfwSetWindowUserPointer(state->window, window_user_data);
    glfwSetKeyCallback(state->window, key_callback);
    glfwSetWindowSizeCallback(state->window, window_resize_callback);
    glfwSetFramebufferSizeCallback(state->window, framebuffer_resize_callback);

    return true;
}

bool acquire_next_image(RendererState* state) {
    VkResult result = vkAcquireNextImageKHR(state->device, state->swapchain, 1000000000, state->acquire_semaphores[state->last_image_index], VK_NULL_HANDLE, &state->image_index);
    if (result != VK_SUCCESS) {
	std::cout << "Error: failed to acquire next image" << std::endl;
	return false;
    }

    return true;
}

bool wait_for_fence(RendererState* state) {
    VkResult result = vkWaitForFences(state->device, 1, state->submissions[state->image_index].fence, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS) {
	std::cout << "Failed to wait for fence (" << result << ")" << std::endl;
	return false;
    }

    result = vkResetFences(state->device, 1, state->submissions[state->image_index].fence);
    if (result != VK_SUCCESS) {
	return false;
    }

    return true;
}

void update_time(Time* time) {
    time->end = std::chrono::high_resolution_clock::now();
    time->delta_time = std::chrono::duration<float, std::ratio<1>>(time->end - time->start).count();
    time->cumulated_time += time->delta_time;
    time->start = std::chrono::high_resolution_clock::now();
}

void update_camera(RendererState* state, Time* time) {
    state->camera.position = new_vec3f(2.0f * sin(PI_2 * time->cumulated_time), 1.0f, 2.0f * cos(PI_2 * time->cumulated_time));
    state->camera_data.context.view = look_at(state->camera.position, state->camera.center, new_vec3f(0.0f, 1.0f, 0.0f));
}


void update(RendererState* state, Time* time) {
    update_time(time);
    update_camera(state, time);
    memcpy(state->camera_data.allocations[state->image_index].data, &state->camera_data.context, sizeof(Context));
}

void render(RendererState* state) {
    VkCommandBuffer command_buffer = {};

    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = state->command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    VkResult allocate_result = vkAllocateCommandBuffers(state->device, &allocate_info, &command_buffer);
    if (allocate_result != VK_SUCCESS) {
	state->crashed = true;
	return;
    }

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkResult result = vkBeginCommandBuffer(command_buffer, &begin_info);
    if (result != VK_SUCCESS) {
	state->crashed = true;
	return;
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
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->pipeline_layout, 0, 1, &state->camera_data.descriptor_sets[state->image_index], 0, nullptr);
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &state->square_entity.buffer, &offset);
    vkCmdDraw(command_buffer, 6, 1, 0, 0);

    vkCmdBindVertexBuffers(command_buffer, 0, 1, &state->cube_entity.buffer, &offset);
    vkCmdDraw(command_buffer, 36, 1, 0, 0);

    vkCmdEndRenderPass(command_buffer);
    vkEndCommandBuffer(command_buffer);

    VkPipelineStageFlags stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &state->acquire_semaphores[state->last_image_index];
    submit_info.pWaitDstStageMask = &stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &state->present_semaphores[state->image_index];

    if (state->submissions[state->image_index].command_buffer) {
	vkFreeCommandBuffers(state->device, state->command_pool, 1, &state->submissions[state->image_index].command_buffer);
    }

    state->submissions[state->image_index].command_buffer = command_buffer;

    result = vkQueueSubmit(state->graphics_queue, 1, &submit_info, *state->submissions[state->image_index].fence);
    if (result != VK_SUCCESS) {
	state->crashed = true;
	return;
    }

    VkResult present_result;
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &state->present_semaphores[state->image_index];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &state->swapchain;
    present_info.pImageIndices = &state->image_index;
    present_info.pResults = &present_result;

    result = vkQueuePresentKHR(state->present_queue, &present_info);
    if (result != VK_SUCCESS) {
	state->crashed;
	return;
    }

    state->last_image_index = state->image_index;
}


void do_frame(RendererState* state, WindowUserData* window_user_data, Time* time) {
    if (!handle_swapchain_recreation(state, window_user_data)) {
	state->crashed = true;
	return;
    }

    if (!acquire_next_image(state)) {
	state->crashed = true;
	return;
    }

    if (!wait_for_fence(state)) {
	state->crashed = true;
	return;
    }

    update(state, time);
    render(state);
}

void destroy_square_entity(RendererState* state, bool verbose = true) {
    if (state->square_entity.buffer) {
	if (verbose) {
	    std::cout << "Destroying square entity buffer (" << state->square_entity.buffer << ")" << std::endl;
	}
	vkDestroyBuffer(state->device, state->square_entity.buffer, nullptr);
	free(&state->memory_manager, &state->square_entity.allocation);
    }
}

void destroy_cube_entity(RendererState* state, bool verbose = true) {
    if (state->square_entity.buffer) {
	if (verbose) {
	    std::cout << "Destroying cube entity buffer (" << state->cube_entity.buffer << ")" << std::endl;
	}
	vkDestroyBuffer(state->device, state->cube_entity.buffer, nullptr);
	free(&state->memory_manager, &state->cube_entity.allocation);
    }
}

void destroy_camera(RendererState* state, bool verbose = true) {
    if (state->camera_data.buffers) {
	for (int i = 0;i < state->swapchain_image_count;i++) {
	    if (verbose) {
		std::cout << "Destroying camera data buffer (" << state->camera_data.buffers[i] << ")" << std::endl;
	    }
	    vkDestroyBuffer(state->device, state->camera_data.buffers[i], nullptr);
	}

	free_null(state->camera_data.buffers);
    }

    if (state->camera_data.allocations) {
	for (int i = 0;i < state->swapchain_image_count;i++) {
	    free(&state->memory_manager, &state->camera_data.allocations[i]);
	}
	free_null(state->camera_data.allocations);
    }
}

void cleanup(RendererState* state) {
    for (int i = 0;i < state->swapchain_image_count;++i) {
	if (state->fences) {
	    vkWaitForFences(state->device, 1, &state->fences[i], VK_TRUE, 1000000000);
	}
    }

    destroy_window(state, true);
    glfwTerminate();

    destroy_camera(state, true);
    destroy_square_entity(state, true);
    destroy_cube_entity(state, true);

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
    time.start = std::chrono::high_resolution_clock::now();
    fps_counter.start = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(state.window) && !state.crashed) {
	do_input(&input);
	do_frame(&state, &window_user_data, &time);

	update_fps_counter(state.window, &fps_counter);
    }

    cleanup(&state);
    return state.crashed ? -1 : 0;
}
