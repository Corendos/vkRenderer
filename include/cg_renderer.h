#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "cg_benchmark.h"
#include "cg_shaders.h"
#include "cg_texture.h"
#include "cg_memory.h"
#include "cg_math.h"
#include "cg_camera.h"
#include "cg_gui.h"
#include "cg_memory_arena.h"
#include "cg_fonts.h"

#define MAX_ENTITY_COUNT 1024
#define MAIN_ARENA_SIZE MB(256)

enum DescriptorSetLayoutName {
    CameraDescriptorSetLayout,
    TransformDescriptorSetLayout,
    CountDescriptorSetLayout
};

struct PhysicalDeviceSelection {
    VkPhysicalDevice device;
    u32 graphics_queue_family_index;
    u32 transfer_queue_family_index;
    u32 compute_queue_family_index;
    u32 present_queue_family_index;
};

struct CommandBufferSubmission {
    VkCommandBuffer command_buffer;
    VkFence* fence;
};

struct Vertex {
    Vec3f position;
    Vec3f color;
};

struct Entity {
    u32 id;
    VkBuffer buffer;
    AllocatedMemoryChunk allocation;
    u32 size;
    u32 offset;
    
    Mat4f *transform;
};

struct EntityResources {
    VkBuffer* buffers;
    VkDescriptorSet* descriptor_sets;
    AllocatedMemoryChunk* allocations;
    Mat4f transforms[MAX_ENTITY_COUNT];
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
    u32 swapchain_image_count;
    VkImage* swapchain_images;
    VkImageView* swapchain_image_views;
    VkExtent2D swapchain_extent;
    bool crashed;
    bool skip_image;
    VkSemaphore* present_semaphores;
    VkSemaphore* acquire_semaphores;
    u32 current_semaphore_index;
    VkFence* submit_fences;
    VkFence* acquire_fences;
    VkCommandPool command_pool;
    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout descriptor_set_layouts[CountDescriptorSetLayout];
    VkDescriptorPool descriptor_pool;
    VkRenderPass renderpass;
    VkPipeline pipeline;
    VkImage* depth_images;
    AllocatedMemoryChunk* depth_image_allocations;
    VkImageView* depth_image_views;
    VkFramebuffer* framebuffers;
    
    Camera camera;
    CameraResources camera_resources;
    
    CommandBufferSubmission* submissions;
    
    u32 image_index;
    ShaderCatalog shader_catalog;
    MemoryManager memory_manager;
    
    TextureCatalog texture_catalog;
    FontCatalog font_catalog;
    FontAtlasCatalog font_atlas_catalog;
    
    bool cursor_locked;
    
    GuiState gui_state;
    GuiResources gui_resources;
    
    Entity entities[MAX_ENTITY_COUNT];
    EntityResources entity_resources;
    u32 entity_count;
    
    MemoryArena temporary_storage;
    MemoryArena main_arena;
    
    u64 counter;
    u64 updater;
};

#endif
