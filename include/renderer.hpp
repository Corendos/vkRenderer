#ifndef __RENDERER_HPP__
#define __RENDERER_HPP__

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "shaders.hpp"
#include "memory.hpp"
#include "math.hpp"

struct PhysicalDeviceSelection {
    VkPhysicalDevice device;
    uint32_t graphics_queue_family_index;
    uint32_t transfer_queue_family_index;
    uint32_t compute_queue_family_index;
    uint32_t present_queue_family_index;
};

struct CommandBufferSubmission {
    VkCommandBuffer command_buffer;
    VkFence* fence;
};


struct Vertex {
    Vec3f position;
    Vec3f color;
};

struct Context {
    Mat4f projection;
    Mat4f view;
};

struct RendererState {
    GLFWwindow* window;
    VkInstance instance;
    PhysicalDeviceSelection selection;
    VkSurfaceKHR surface;
    VkDevice device;
    VkQueue graphics_queue;
    VkQueue transfer_queue;
    VkQueue compute_queue;
    VkQueue present_queue;
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR present_mode;
    VkSwapchainKHR swapchain;
    uint32_t swapchain_image_count;
    VkImage* swapchain_images;
    VkImageView* swapchain_image_views;
    VkExtent2D swapchain_extent;
    bool crashed;
    VkSemaphore* present_semaphores;
    VkSemaphore* acquire_semaphores;
    VkCommandPool command_pool;
    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet* descriptor_sets;
    VkRenderPass renderpass;
    VkPipeline pipeline;
    VkFramebuffer* framebuffers;

    VkFence* fences;
    CommandBufferSubmission* submissions;
    uint32_t last_image_index;
    uint32_t image_index;
    VkBuffer buffer;
    AllocatedMemoryChunk buffer_allocation;
    VkBuffer* context_buffers;
    AllocatedMemoryChunk* context_buffer_allocations;

    ShaderCatalog shader_catalog;
    MemoryManager memory_manager;
    Context context;
};

#endif
