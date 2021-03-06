#ifndef __VK_HELPER_H__
#define __VK_HELPER_H__

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "cg_renderer.h"

const char* get_device_type_str(VkPhysicalDeviceType type);
const char* vk_error_code_str(VkResult result);
VkFormat get_format_from_channels(u32 channels);
bool check_queue_availability(PhysicalDeviceSelection* selection, VkSurfaceKHR surface);
u32 get_device_type_score(VkPhysicalDeviceType type);
bool select_physical_device(RendererState* state);
u32 get_surface_format_score(VkSurfaceFormatKHR surface_format);
bool select_surface_format(RendererState* state);
u32 get_present_mode_score(VkPresentModeKHR present_mode);
bool select_present_mode(RendererState* state);
bool check_required_layers();
bool check_required_instance_extensions(const char** extensions, u32 count);
bool check_required_device_extensions(VkPhysicalDevice physical_device);

bool create_instance(VkInstance* instance);
bool create_device_and_queues(RendererState* state);
bool create_swapchain(RendererState* state);
bool get_swapchain_images(RendererState* state);
bool create_swapchain_image_views(RendererState* state);
bool create_depth_images(RendererState* state);
bool create_semaphore(RendererState* state);
bool create_submit_fences(RendererState* state);
bool create_acquire_fences(RendererState* state);
bool create_fences(RendererState* state);
bool create_command_pool(RendererState* state);
bool create_descriptor_set_layout(RendererState* state);
bool create_pipeline_layout(RendererState* state);
bool create_render_pass(RendererState* state);
bool create_graphics_pipeline(RendererState* state);
bool create_gui_graphics_pipeline(RendererState* state);
bool create_framebuffers(RendererState* state);
bool create_descriptor_pool(RendererState* state);
bool allocate_descriptor_set(RendererState* state);
bool create_context_ubo(RendererState* state);
void update_descriptor_set(RendererState* state);

void destroy_window(RendererState* state, bool verbose = false);
void destroy_framebuffers(RendererState* state, bool verbose = false);
void destroy_pipeline(RendererState* state, bool verbose = false);
void destroy_gui_pipeline(RendererState* state, bool verbose = false);
void destroy_renderpass(RendererState* state, bool verbose = false);
void destroy_descriptor_set_layout(RendererState* state, bool verbose = false);
void destroy_descriptor_pool(RendererState* state, bool verbose = false);
void destroy_pipeline_layout(RendererState* state, bool verbose = false);
void destroy_command_pool(RendererState* state, bool verbose = false);
void destroy_fences(RendererState* state, bool verbose = false);
void destroy_submissions(RendererState* state, bool verbose = false);
void destroy_semaphores(RendererState* state, bool verbose = false);
void destroy_depth_images(RendererState* state, bool verbose = false);
void destroy_swapchain(RendererState* state, bool verbose = false);
void destroy_device(RendererState* state, bool verbose = false);
void destroy_surface(RendererState* state, bool verbose = false);
void destroy_instance(RendererState* state, bool verbose = false);
void destroy_swapchain_image_views(RendererState* state, bool verbose = false);

#endif
