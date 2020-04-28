#ifndef __TEXTURE_HPP__
#define __TEXTURE_HPP__

#include <vulkan/vulkan.h>
#include "memory.hpp"

struct RendererState;

struct Texture {
    VkImage image;
    AllocatedMemoryChunk allocation;
    u32 width;
    u32 height;
    u32 channels;
};

struct TextureCatalog {
    Texture* textures;
    u32 count;
    
    VkBuffer staging_buffer;
    AllocatedMemoryChunk allocation;
    VkCommandPool command_pool;
};

bool create_texture_catalog_staging_buffer(RendererState* state);
bool create_texture_catalog_command_pool(RendererState* state);
bool init_texture_catalog(RendererState* state, u32 size);
void destroy_texture_catalog_staging_buffer(RendererState* state, bool verbose = false);
void destroy_texture_catalog_command_pool(RendererState* state, bool verbose = false);
void cleanup_texture_catalog(RendererState* state, bool verbose = false);
bool load_texture(RendererState* state, const char* filename, const char* texture_name);

#endif
