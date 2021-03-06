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
#include "cg_material.h"
#include "cg_memory_arena.h"
#include "cg_fonts.h"
#include "cg_vertex.h"

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

struct EntityTransformData {
    Mat4f model_matrix;
    Mat4f normal_matrix;
};

struct Entity {
    u32 id;
    VkBuffer buffer;
    AllocatedMemoryChunk allocation;
    u32 size;
    u32 offset;
    
    EntityTransformData *transform_data;
};

struct EntityResources {
    VkBuffer* buffers;
    VkDescriptorSet* descriptor_sets;
    AllocatedMemoryChunk* allocations;
    EntityTransformData transform_data[MAX_ENTITY_COUNT];
};

struct TempData {
    u64 counter;
    u64 updater;
    
    u32 frame_count_update;
    
    u32 light_entity_id;
    
    i32 rotation_speed;
    f32 current_angle;
    bool minus_button_status;
    bool plus_button_status;
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
    
    MaterialCatalog material_catalog;
    
    bool cursor_locked;
    
    GuiState gui_state;
    GuiResources gui_resources;
    
    Entity entities[MAX_ENTITY_COUNT];
    EntityResources entity_resources;
    u32 entity_count;
    
    MemoryArena temporary_storage;
    MemoryArena main_arena;
    
    TempData temp_data;
};

#endif
