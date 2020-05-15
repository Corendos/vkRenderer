#include "cg_texture.h"

#include "cg_hash.h"
#include "cg_macros.h"
#include "cg_renderer.h"
#include "cg_vk_helper.h"
#include "cg_utils.h"

inline bool create_texture_catalog_staging_buffer(RendererState* state) {
    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = 4096 * 4096 * 4; // Enough space to store a 4096 by 4096 RGBA8 texture
    create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &state->selection.graphics_queue_family_index;
    
    VkResult result = vkCreateBuffer(state->device, &create_info, nullptr, &state->texture_catalog.staging_buffer);
    
    if (result != VK_SUCCESS) {
        println("vkCreateBuffer returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    VkMemoryRequirements requirements = {};
    
    vkGetBufferMemoryRequirements(state->device, state->texture_catalog.staging_buffer, &requirements);
    
    VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    
    if(!allocate(&state->memory_manager, state->device, requirements, memory_flags, &state->texture_catalog.allocation)) {
        return false;
    }
    
    result = vkBindBufferMemory(state->device, state->texture_catalog.staging_buffer, state->texture_catalog.allocation.device_memory, state->texture_catalog.allocation.offset);
    if (result != VK_SUCCESS) {
        println("vkBindBufferMemory returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline bool create_texture_catalog_command_pool(RendererState* state) {
    VkCommandPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    create_info.queueFamilyIndex = state->selection.graphics_queue_family_index;
    
    VkResult result = vkCreateCommandPool(state->device, &create_info, nullptr, &state->texture_catalog.command_pool);
    if (result != VK_SUCCESS) {
        println("vkCreateCommandPool returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline bool init_texture_catalog(TextureCatalog* catalog, u32 size, RendererState* state) {
    catalog->textures = (Texture*)calloc(size, sizeof(Texture));
    catalog->count = size;
    
    if (!create_texture_catalog_staging_buffer(state)) {
        println("Error: failed to create staging buffer");
        return false;
    }
    
    if (!create_texture_catalog_command_pool(state)) {
        println("Error: failed to create command pool");
        return false;
    }
    
    return true;
}

inline void destroy_texture_catalog_staging_buffer(RendererState* state, bool verbose) {
    if (state->texture_catalog.staging_buffer) {
        if (verbose) {
            println("Destroying texture catalog staging buffer (%p)", state->texture_catalog.staging_buffer);
        }
        vkDestroyBuffer(state->device, state->texture_catalog.staging_buffer, nullptr);
    }
    if (verbose) {
        println("");
    }
}

inline void destroy_texture_catalog_command_pool(RendererState* state, bool verbose) {
    if (state->texture_catalog.command_pool) {
        if (verbose) {
            println("Destroying texture catalog command pool (%p)", state->texture_catalog.command_pool);
        }
        vkDestroyCommandPool(state->device, state->texture_catalog.command_pool, nullptr);
    }
    if (verbose) {
        println("");
    }
}

inline void destroy_textures(RendererState* state, bool verbose) {
    if (verbose) {
        println("Destroying textures");
    }
    TextureCatalog* catalog = &state->texture_catalog;
    if (catalog->textures) {
        for (int i = 0;i < catalog->count;++i) {
            if (catalog->textures[i].image) {
                if (verbose) {
                    println("    Destroying image (%p)", catalog->textures[i].image);
                }
                vkDestroyImage(state->device, catalog->textures[i].image, nullptr);
                
                free(&state->memory_manager, &catalog->textures[i].allocation);
            }
        }
        
        free_null(catalog->textures);
    }
    if (verbose) {
        println("");
    }
}

inline void cleanup_texture_catalog(RendererState* state, bool verbose) {
    // @Note: don't forget to destroy image handle eventually
    destroy_texture_catalog_staging_buffer(state, verbose);
    destroy_texture_catalog_command_pool(state, verbose);
    destroy_textures(state, verbose);
}

inline bool create_texture(RendererState* state, Texture* texture, u32 width, u32 height, u32 channels) {
    texture->width    = width;
    texture->height   = height;
    texture->channels = channels;
    
    VkImageCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.imageType = VK_IMAGE_TYPE_2D;
    create_info.format = get_format_from_channels(channels);
    create_info.extent = { width, height, 1 };
    create_info.mipLevels = 1;
    create_info.arrayLayers = 1;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &state->selection.graphics_queue_family_index;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    
    VkResult result = vkCreateImage(state->device, &create_info, nullptr, &texture->image);
    if (result != VK_SUCCESS) {
        println("vkCreateImage returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    VkMemoryRequirements requirements = {};
    
    vkGetImageMemoryRequirements(state->device, texture->image, &requirements);
    
    VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    if(!allocate(&state->memory_manager, state->device, requirements, memory_flags, &texture->allocation)) {
        println("Error: failed to allocate for image");
        return false;
    }
    
    result = vkBindImageMemory(state->device, texture->image, texture->allocation.device_memory, texture->allocation.offset);
    if (result != VK_SUCCESS) {
        println("vkBindImageMemory returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline bool copy_from_staging_buffer(RendererState* state, Texture* texture) {
    TextureCatalog* catalog = &state->texture_catalog;
    
    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = catalog->command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    
    VkCommandBuffer command_buffer = {};
    
    VkResult result = vkAllocateCommandBuffers(state->device, &allocate_info, &command_buffer);
    if (result != VK_SUCCESS) {
        println("vkAllocateCommandBuffers returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    result = vkBeginCommandBuffer(command_buffer, &begin_info);
    if (result != VK_SUCCESS) {
        println("vkBeginCommandBuffer returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;
    
    VkImageMemoryBarrier first_barrier = {};
    first_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    first_barrier.srcAccessMask = 0;
    first_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    first_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    first_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    first_barrier.srcQueueFamilyIndex = state->selection.graphics_queue_family_index;
    first_barrier.dstQueueFamilyIndex = state->selection.graphics_queue_family_index;
    first_barrier.image = texture->image;
    first_barrier.subresourceRange = subresource_range;
    
    vkCmdPipelineBarrier(command_buffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &first_barrier);
    
    
    VkImageSubresourceLayers subresource_layers = {};
    subresource_layers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_layers.mipLevel = 0;
    subresource_layers.baseArrayLayer = 0;
    subresource_layers.layerCount = 1;
    
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.imageSubresource = subresource_layers;
    region.imageExtent = { texture->width, texture->height, 1 };
    
    vkCmdCopyBufferToImage(command_buffer,
                           catalog->staging_buffer,
                           texture->image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &region);
    
    VkImageMemoryBarrier second_barrier = {};
    second_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    second_barrier.srcAccessMask = 0;
    second_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    second_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    second_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    second_barrier.srcQueueFamilyIndex = state->selection.graphics_queue_family_index;
    second_barrier.dstQueueFamilyIndex = state->selection.graphics_queue_family_index;
    second_barrier.image = texture->image;
    second_barrier.subresourceRange = subresource_range;
    
    vkCmdPipelineBarrier(command_buffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &second_barrier);
    
    result = vkEndCommandBuffer(command_buffer);
    if (result != VK_SUCCESS) {
        println("vkEndCommandBuffer returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    
    result = vkQueueSubmit(state->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    if (result != VK_SUCCESS) {
        println("vkQueueSubmit returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    result = vkQueueWaitIdle(state->graphics_queue);
    if (result != VK_SUCCESS) {
        println("vkQueueWaitIdle returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    vkFreeCommandBuffers(state->device, catalog->command_pool, 1, &command_buffer);
    
    return true;
}

inline Texture* get_texture_from_catalog(TextureCatalog* catalog, const char* texture_name) {
    if (catalog->count <= 0) {
        println("Error: catalog is not allocated.");
        return 0;
    }
    
    u32 index = hash(texture_name) % catalog->count;
    
    return &catalog->textures[index];
}

inline bool load_texture(RendererState* state, const u8* pixels, u32 width, u32 height, u32 channels, const char* texture_name) {
    TextureCatalog* catalog = &state->texture_catalog;
    if (catalog->count <= 0) {
        println("Error: catalog is not allocated.");
        return false;
    }
    
    u32 index = hash(texture_name) % catalog->count;
    
    if (catalog->textures[index].image != 0) {
        println("Error: texture name collision in hashtable.");
        return false;
    }
    
    // Copy the data to the staging buffer
    memcpy(catalog->allocation.data, pixels, width * height * channels);
    
    if (!create_texture(state, &catalog->textures[index], width, height, channels)) {
        println("Error: failed to create texture");
        return false;
    }
    
    if(!copy_from_staging_buffer(state, &catalog->textures[index])) {
        println("Error: failed to copy from staging buffer");
        return false;
    }
    
    return true;
}

inline bool load_texture_from_filename(RendererState *state, const char* filename, const char* texture_name) {
    char full_filename[256] = {0};
    // @Warning: this is unchecked
    get_full_path_from_root(filename, full_filename);
    
    int width, height, channels;
    u8* pixels = stbi_load(full_filename, &width, &height, &channels, 4);
    if (!pixels) {
        println("Error: failed to load image '%s'.", filename);
        return false;
    }
    
    if (!load_texture(state, pixels, width, height, channels, texture_name)) {
        stbi_image_free(pixels);
        return false;
    }
    
    stbi_image_free(pixels);
    return true;
}
