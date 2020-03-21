#include <iostream>
#include <cstdlib>
#include <string>
#include <thread>
#include <cstring>
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

	if(!create_graphics_pipeline(state)) {
	    std::cout << "Failed to recreate pipeline" << std::endl;
	}

	if(!create_framebuffers(state)) {
	    std::cout << "Failed to recreate framebuffers" << std::endl;
	}

	window_user_data->swapchain_need_recreation = false;
	state->context.projection = perspective(70.0f, (float)state->swapchain_extent.width / (float)state->swapchain_extent.height, 0.1f, 100.0f);
    }

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

    if (!allocate_descriptor_set(state)) {
	return false;
    } else {
	std::cout << "descriptor set init: success" << std::endl;
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

    uint32_t allocation_size = mega(64);
    uint32_t min_page_size = kilo(4);
    uint32_t page_count = allocation_size / min_page_size;

    if (!init_memory(&state->memory_manager, allocation_size, min_page_size, state->selection.device)) {
	return false;
    } else {
	std::cout << "memory manager init: success" << std::endl;
    }

    if (!create_vertex_buffer(state)) {
	return false;
    } else {
	std::cout << "buffer creation : success" << std::endl;
    }

    if (!create_context_ubo(state)) {
	return false;
    } else {
	std::cout << "context ubo creation : success" << std::endl;
    }

    update_descriptor_set(state);

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

void update(RendererState* state) {
    memcpy(state->context_buffer_allocations[state->image_index].data, &state->context, sizeof(Context));
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

    VkClearValue clear_color = {0.05f, 0.05f, 0.05f, 1.0f};

    VkRenderPassBeginInfo renderpass_begin_info = {};
    renderpass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpass_begin_info.renderPass = state->renderpass;
    renderpass_begin_info.framebuffer = state->framebuffers[state->image_index];
    renderpass_begin_info.renderArea.offset = {0, 0};
    renderpass_begin_info.renderArea.extent = state->swapchain_extent;
    renderpass_begin_info.clearValueCount = 1;
    renderpass_begin_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(command_buffer, &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->pipeline);
    VkDeviceSize offset = 0;
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->pipeline_layout, 0, 1, &state->descriptor_sets[state->image_index], 0, nullptr);
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &state->buffer, &offset);
    vkCmdDraw(command_buffer, 6, 1, 0, 0);

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


void do_frame(RendererState* state, WindowUserData* window_user_data) {
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

    update(state);
    render(state);
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
    fps_counter.start = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(state.window) && !state.crashed) {
	do_input(&input);
	do_frame(&state, &window_user_data);

	update_fps_counter(state.window, &fps_counter);
    }

    cleanup(&state);
    return state.crashed ? -1 : 0;
}
