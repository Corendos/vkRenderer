#ifndef __RENDERER_HPP__
#define __RENDERER_HPP__

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "shaders.hpp"
#include "memory.hpp"
#include "math.hpp"
#include "camera.hpp"

#define MAX_ENTITY_COUNT 10240

enum DescriptorSetLayoutName { CameraDescriptorSetLayout, TransformDescriptorSetLayout, CountDescriptorSetLayout };

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

struct SquareEntity {
    VkDescriptorSet descriptor_set;
    VkBuffer buffer;
    AllocatedMemoryChunk allocation;
};

struct CubeEntity {
    VkDescriptorSet descriptor_set;
    VkBuffer buffer;
    AllocatedMemoryChunk allocation;
};

struct Entity {
    uint32_t id;
    VkBuffer buffer;
    AllocatedMemoryChunk allocation;
    uint32_t size;
    uint32_t offset;

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
    uint32_t swapchain_image_count;
    VkImage* swapchain_images;
    VkImageView* swapchain_image_views;
    VkExtent2D swapchain_extent;
    bool crashed;
    VkSemaphore* present_semaphores;
    VkSemaphore* acquire_semaphores;
    VkFence* fences;
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

    SquareEntity square_entity;
    CubeEntity cube_entity;
    Camera camera;
    CameraResources camera_resources;

    CommandBufferSubmission* submissions;

    uint32_t last_image_index;
    uint32_t image_index;
    ShaderCatalog shader_catalog;
    MemoryManager memory_manager;

    bool cursor_locked;

    Entity entities[MAX_ENTITY_COUNT];
    EntityResources entity_resources;
    uint32_t entity_count;
};

#endif
