#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <vulkan/vulkan.h>
#include "memory.h"

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
bool init_texture_catalog(TextureCatalog* catalog, u32 size, RendererState* state);
void destroy_texture_catalog_staging_buffer(RendererState* state, bool verbose = false);
void destroy_texture_catalog_command_pool(RendererState* state, bool verbose = false);
void cleanup_texture_catalog(RendererState* state, bool verbose = false);
Texture* get_texture_from_catalog(TextureCatalog* catalog, const char* texture_name);
bool load_texture(RendererState* state, const u8* pixels, u32 width, u32 height, u32 channels, const char* texture_name);
bool load_texture_from_filename(RendererState* state, const char* filename, const char* texture_name);

#endif
