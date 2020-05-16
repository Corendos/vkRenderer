#include "cg_fonts.h"

#include "stdlib.h"
#include "cg_files.h"
#include "cg_texture.h"

inline u8* load_font_data_from_file(ConstString* filename, MemoryArena* storage) {
    FILE* font_file = fopen(filename->str, "rb");
    
    if (font_file == 0) return 0;
    
    u32 file_size = get_file_size(font_file);
    
    u8* font_data = (u8*)allocate(storage, file_size);
    
    copy_file_to(font_file, font_data, file_size);
    fclose(font_file);
    
    return font_data;
}

inline bool load_font_from_data(Font* font, ConstString* font_name, const u8* font_data, MemoryArena* storage) {
    if (stbtt_InitFont(&font->font_info, font_data, 0) == 0) {
        println("Error: failed to initialize font");
        return false;
    }
    
    font->name = push_string_copy(storage, font_name);
    
    return true;
}

inline bool load_font_from_file(Font* font, ConstString* font_name,
                                ConstString* filename, MemoryArena* storage){
    u8* font_data = load_font_data_from_file(filename, storage);
    if (font_data == 0) return false;
    
    return load_font_from_data(font, font_name, font_data, storage);
}


// Font Catalog //
inline bool init_font_catalog(FontCatalog* catalog, u32 size, MemoryArena* storage) {
    catalog->entries = (FontEntry**)zero_allocate(storage, size * sizeof(FontEntry*));
    catalog->size = size;
    
    return (catalog->entries != 0);
}

inline bool add_font_to_catalog(FontCatalog* catalog, Font* font, MemoryArena* storage) {
    u64 font_hash = hash(font->name);
    
    u32 index = font_hash % catalog->size;
    
    FontEntry* new_entry = (FontEntry*)zero_allocate(storage, sizeof(FontEntry));
    
    new_entry->font.font_info = font->font_info;
    new_entry->font.name = font->name;
    new_entry->hash = font_hash;
    
    new_entry->next = catalog->entries[index];
    catalog->entries[index] = new_entry;
    
    return true;
}

inline bool load_and_add_font_to_catalog(FontCatalog* catalog, ConstString* font_name, ConstString* filename, MemoryArena* storage) {
    Font font = {};
    if (!load_font_from_file(&font, font_name, filename, storage)) {
        return false;
    }
    
    return add_font_to_catalog(catalog, &font, storage);
}

inline Font* get_font_from_catalog(FontCatalog* catalog, ConstString* font_name) {
    u64 font_hash = hash(*font_name);
    
    u32 index = font_hash % catalog->size;
    
    FontEntry* entry = catalog->entries[index];
    
    
    while (entry) {
        if (entry->hash == font_hash &&
            strcmp(entry->font.name.str, font_name->str) == 0) {
            return &entry->font;
        }
        entry = entry->next;
    }
    
    return 0;
}

inline u64 get_font_hash(ConstString* font_name, u32 font_size) {
    u8 data[64] = {};
    memcpy(data, font_name->str, font_name->size);
    memcpy(data + font_name->size, &font_size, sizeof(font_size));
    
    return hash(data, font_name->size + sizeof(font_size));
}


// Font Atlas Catalog //
inline bool init_font_atlas_catalog_resources(FontAtlasCatalog* catalog, RendererState* state, MemoryArena* storage) {
    FontAtlasCatalogResources* resources = &catalog->resources;
    // Create image
    VkImageCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.imageType = VK_IMAGE_TYPE_2D;
    create_info.format = VK_FORMAT_R8_UNORM;
    create_info.extent = { FONT_ATLAS_SIZE, FONT_ATLAS_SIZE, 1 };
    create_info.mipLevels = 1;
    create_info.arrayLayers = catalog->size;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &state->selection.graphics_queue_family_index;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    
    VkResult result = vkCreateImage(state->device, &create_info, nullptr, &resources->texture_array);
    
    if (result != VK_SUCCESS) {
        println("vkCreateImage returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    // Allocate memory
    VkMemoryRequirements requirements = {};
    vkGetImageMemoryRequirements(state->device, resources->texture_array, &requirements);
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    if (!allocate(&state->memory_manager, state->device, requirements, properties, &resources->texture_array_allocation)) {
        println("Error: failed to allocate memory for font atlas texture.");
        return false;
    }
    
    // Bind memory to image
    result = vkBindImageMemory(state->device,
                               resources->texture_array,
                               resources->texture_array_allocation.device_memory,
                               resources->texture_array_allocation.offset);
    if (result != VK_SUCCESS) {
        println("Error: failed to bind image memory.");
        return false;
    }
    
    // Create staging buffer
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = FONT_ATLAS_SIZE * FONT_ATLAS_SIZE;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 1;
    buffer_create_info.pQueueFamilyIndices = &state->selection.graphics_queue_family_index;
    
    result = vkCreateBuffer(state->device, &buffer_create_info, nullptr, &resources->staging_buffer);
    if (result != VK_SUCCESS) {
        println("Error: failed to create staging buffer.");
        return false;
    }
    
    vkGetBufferMemoryRequirements(state->device, resources->staging_buffer, &requirements);
    properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    
    if (!allocate(&state->memory_manager, state->device, requirements, properties, &resources->staging_buffer_allocation)) {
        println("Error: failed to allocate memory for staging buffer.");
        return false;
    }
    
    result = vkBindBufferMemory(state->device,
                                resources->staging_buffer,
                                resources->staging_buffer_allocation.device_memory,
                                resources->staging_buffer_allocation.offset);
    
    if (result != VK_SUCCESS) {
        println("Error: failed to bind buffer memory.");
        return false;
    }
    
    // Allocate descriptor set
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.descriptorPool = state->descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &state->gui_resources.descriptor_set_layout;
    
    result = vkAllocateDescriptorSets(state->device, &descriptor_set_allocate_info, &resources->descriptor_set);
    
    if (result != VK_SUCCESS) {
        println("Error: failed to allocate descriptor set.");
        return false;
    }
    
    // Create image view
    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = catalog->size;
    
    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = resources->texture_array;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    image_view_create_info.format = VK_FORMAT_R8_UNORM;
    image_view_create_info.subresourceRange = subresource_range;
    
    result = vkCreateImageView(state->device, &image_view_create_info, nullptr, &resources->texture_array_image_view);
    if (result != VK_SUCCESS) {
        println("vkCreateImageView returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    // Create sampler
    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter = VK_FILTER_NEAREST;
    sampler_create_info.minFilter = VK_FILTER_NEAREST;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    
    result = vkCreateSampler(state->device, &sampler_create_info, nullptr, &resources->texture_array_sampler);
    if (result != VK_SUCCESS) {
        println("vkCreateSampler returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    // Update descriptor set
    VkDescriptorImageInfo image_info = {};
    image_info.sampler = resources->texture_array_sampler;
    image_info.imageView = resources->texture_array_image_view;
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    VkWriteDescriptorSet write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = resources->descriptor_set;
    write_descriptor_set.dstBinding = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_set.pImageInfo = &image_info;
    
    vkUpdateDescriptorSets(state->device, 1, &write_descriptor_set, 0, nullptr);
    
    // Create command pool
    VkCommandPoolCreateInfo command_pool_create_info = {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    command_pool_create_info.queueFamilyIndex = state->selection.graphics_queue_family_index;
    
    result = vkCreateCommandPool(state->device, &command_pool_create_info, nullptr, &resources->command_pool);
    if (result != VK_SUCCESS) {
        println("Error: failed to create font atlas catalog command pool.");
        return false;
    }
    
    // Create fence
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    
    result = vkCreateFence(state->device, &fence_create_info, nullptr, &resources->transfer_fence);
    if (result != VK_SUCCESS) {
        println("Error: failed to create font atlas transfer fence.");
        return false;
    }
    
    // Copy blank texture in every slot
    if (!resources->staging_buffer_allocation.mappable) {
        println("Error: staging buffer is not mappable.");
        return false;
    }
    
    memset(resources->staging_buffer_allocation.data, 255, FONT_ATLAS_SIZE * FONT_ATLAS_SIZE);
    
    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = resources->command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    
    VkCommandBuffer command_buffer = {};
    
    result = vkAllocateCommandBuffers(state->device, &allocate_info, &command_buffer);
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
    
    subresource_range = {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = catalog->size;
    
    VkImageMemoryBarrier first_barrier = {};
    first_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    first_barrier.srcAccessMask = 0;
    first_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    first_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    first_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    first_barrier.srcQueueFamilyIndex = state->selection.graphics_queue_family_index;
    first_barrier.dstQueueFamilyIndex = state->selection.graphics_queue_family_index;
    first_barrier.image = resources->texture_array;
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
    subresource_layers.layerCount = 1;
    
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.imageExtent = { FONT_ATLAS_SIZE, FONT_ATLAS_SIZE, 1 };
    
    // NOTE: apparently you can't copy the same src buffer to different dst image in one shot
    for (u32 i = 0;i < catalog->size;++i) {
        subresource_layers.baseArrayLayer = i;
        region.imageSubresource = subresource_layers;
        
        vkCmdCopyBufferToImage(command_buffer,
                               resources->staging_buffer,
                               resources->texture_array,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &region);
    }
    
    VkImageMemoryBarrier second_barrier = {};
    second_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    second_barrier.srcAccessMask = 0;
    second_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    second_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    second_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    second_barrier.srcQueueFamilyIndex = state->selection.graphics_queue_family_index;
    second_barrier.dstQueueFamilyIndex = state->selection.graphics_queue_family_index;
    second_barrier.image = resources->texture_array;
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
    
    result = vkQueueSubmit(state->graphics_queue, 1, &submit_info, resources->transfer_fence);
    if (result != VK_SUCCESS) {
        println("vkQueueSubmit returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    // Wait for fence
    result = vkWaitForFences(state->device, 1, &resources->transfer_fence, VK_TRUE, 1e9);
    if (result != VK_SUCCESS) {
        println("vkWaitForFences returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    result = vkResetFences(state->device, 1, &resources->transfer_fence);
    if (result != VK_SUCCESS) {
        println("vkResetFences resturned (%s)", vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline bool init_font_atlas_catalog(FontAtlasCatalog* catalog, u32 size, RendererState* state, MemoryArena* storage) {
    //catalog->entries = (FontAtlasEntry**)zero_allocate(storage, size * sizeof(FontAtlasEntry*));
    catalog->atlases = (FontAtlas*)zero_allocate(storage, size * sizeof(FontAtlas));
    catalog->size = size;
    
    if (catalog->atlases == 0) {
        println("Error: failed to allocate font atlas array.");
        return false;
    }
    
    if (!init_font_atlas_catalog_resources(catalog, state, storage)) {
        println("Error: failed to init font catalog resources.");
    }
    
    return true;
}

inline void destroy_font_atlas_catalog_resources(RendererState* state, FontAtlasCatalog* catalog, bool verbose) {
    FontAtlasCatalogResources* resources = &catalog->resources;
    
    // Destroy fence
    if (resources->transfer_fence) {
        if (verbose) {
            println("    Destroying fence (%p)", resources->transfer_fence);
        }
        
        vkDestroyFence(state->device, resources->transfer_fence, nullptr);
    }
    
    // Destroy command pool
    if (resources->command_pool) {
        if (verbose) {
            println("    Destroying command pool (%p)", resources->command_pool);
        }
        
        vkDestroyCommandPool(state->device, resources->command_pool, nullptr);
    }
    
    // Destroy sampler
    if (resources->texture_array_sampler) {
        if (verbose) {
            println("    Destroying sampler (%p)", resources->texture_array_sampler);
        }
        vkDestroySampler(state->device, resources->texture_array_sampler, nullptr);
        
    }
    
    // Destroy image view
    if (resources->texture_array_image_view) {
        if (verbose) {
            println("    Destroying image view (%p)", resources->texture_array_image_view);
        }
        vkDestroyImageView(state->device, resources->texture_array_image_view, nullptr);
    }
    
    // Destroy staging buffer
    if (resources->staging_buffer) {
        if (verbose) {
            println("    Destroying staging buffer (%p)", resources->staging_buffer);
        }
        vkDestroyBuffer(state->device, resources->staging_buffer, nullptr);
    }
    
    if (verbose) {
        println("    Freeing staging buffer allocation");
    }
    free(&state->memory_manager, &resources->staging_buffer_allocation);
    
    // Destroy texture array
    if (resources->texture_array) {
        if (verbose) {
            println("    Destroying image (%p)", resources->texture_array);
        }
        
        vkDestroyImage(state->device, resources->texture_array, nullptr);
    }
    
    if (verbose) {
        println("    Freeing font atlas texture allocation");
    }
    free(&state->memory_manager, &resources->texture_array_allocation);
}

inline void destroy_font_atlas_catalog(RendererState* state, FontAtlasCatalog* catalog, bool verbose) {
    if (verbose) {
        println("Destroying font atlas catalog");
    }
    destroy_font_atlas_catalog_resources(state, catalog, verbose);
    
    if (verbose) {
        println("");
    }
}

inline Glyph2 make_glyph_from_packed_char(stbtt_packedchar* packed_char) {
    Glyph2 glyph = {};
    
    glyph.left       = packed_char->x0;
    glyph.top        = packed_char->y0;
    glyph.right      = packed_char->x1;
    glyph.bottom     = packed_char->y1;
    glyph.x_offset   = packed_char->xoff;
    glyph.y_offset   = packed_char->yoff;
    glyph.x_advance  = packed_char->xadvance;
    glyph.x_offset_2 = packed_char->xoff2;
    glyph.y_offset_2 = packed_char->yoff2;
    
    return glyph;
}

inline bool create_font_atlas(Font* font,
                              FontAtlas* font_atlas,
                              u32 font_size,
                              u32 first_unicode_character,
                              u32 character_count,
                              MemoryArena* storage) {
    font_atlas->width  = FONT_ATLAS_SIZE;
    font_atlas->height = FONT_ATLAS_SIZE;
    font_atlas->size = font_size;
    font_atlas->font = font;
    font_atlas->first_unicode_character = first_unicode_character;
    
    font_atlas->glyph_count = character_count;
    font_atlas->glyphs = (Glyph2*)zero_allocate(storage, character_count * sizeof(Glyph2));
    font_atlas->pixels = (u8*)zero_allocate(storage, font_atlas->width * font_atlas->height * sizeof(u8));
    
    TemporaryMemory temporary_memory = make_temporary_memory(storage);
    
    stbtt_packedchar* packed_characters = (stbtt_packedchar*)zero_allocate(&temporary_memory, character_count * sizeof(stbtt_packedchar));
    
    if (packed_characters == 0) return false;
    
    stbtt_pack_context context = {};
    int result = stbtt_PackBegin(&context,
                                 font_atlas->pixels,
                                 font_atlas->width,
                                 font_atlas->height,
                                 0,
                                 1,
                                 nullptr);
    
    if (result == 0) return false;
    stbtt_PackSetSkipMissingCodepoints(&context, 0);
    
    // Generate the font atlas
    result = stbtt_PackFontRange(&context, font->font_info.data, 0, font_size,
                                 first_unicode_character, character_count,
                                 packed_characters);
    
    stbtt_PackEnd(&context);
    
    for (u32 i = 0;i < character_count;++i) {
        Glyph2* glyph = font_atlas->glyphs + i;
        stbtt_packedchar* packed_char = packed_characters + i;
        *glyph = make_glyph_from_packed_char(packed_char);
    }
    
    destroy_temporary_memory(&temporary_memory);
    
    return (result != 0);
}

inline bool copy_font_atlas(FontAtlas* font_atlas,
                            FontAtlasCatalogResources* resources,
                            RendererState* state) {
    memcpy(resources->staging_buffer_allocation.data, font_atlas->pixels, font_atlas->width * font_atlas->height);
    
    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = resources->command_pool;
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
    subresource_range.baseArrayLayer = font_atlas->texture_array_index;
    subresource_range.layerCount = 1;
    
    VkImageMemoryBarrier first_barrier = {};
    first_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    first_barrier.srcAccessMask = 0;
    first_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    first_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    first_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    first_barrier.srcQueueFamilyIndex = state->selection.graphics_queue_family_index;
    first_barrier.dstQueueFamilyIndex = state->selection.graphics_queue_family_index;
    first_barrier.image = resources->texture_array;
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
    subresource_layers.baseArrayLayer = font_atlas->texture_array_index;
    subresource_layers.layerCount = 1;
    
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.imageSubresource = subresource_layers;
    region.imageExtent = { font_atlas->width, font_atlas->height, 1 };
    
    vkCmdCopyBufferToImage(command_buffer,
                           resources->staging_buffer,
                           resources->texture_array,
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
    second_barrier.image = resources->texture_array;
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
    
    result = vkQueueSubmit(state->graphics_queue, 1, &submit_info, resources->transfer_fence);
    if (result != VK_SUCCESS) {
        println("vkQueueSubmit returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    // Wait for fence
    result = vkWaitForFences(state->device, 1, &resources->transfer_fence, VK_TRUE, 1e9);
    if (result != VK_SUCCESS) {
        println("vkWaitForFences returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    result = vkResetFences(state->device, 1, &resources->transfer_fence);
    if (result != VK_SUCCESS) {
        println("vkResetFences resturned (%s)", vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline bool create_and_add_font_atlas(FontCatalog* font_catalog,
                                      ConstString* font_name,
                                      u32 font_size,
                                      FontAtlasCatalog* font_atlas_catalog,
                                      u32 first_unicode_character,
                                      u32 character_count,
                                      RendererState* state,
                                      MemoryArena* arena) {
    if (font_atlas_catalog->size <= 0) {
        println("Error: font catalog not initialized");
        return false;
    }
    
    Font* font = get_font_from_catalog(font_catalog, font_name);
    if (font == 0) {
        println("Error: failed to get font from catalog");
        return false;
    }
    
    FontAtlas* font_atlas = font_atlas_catalog->atlases + font_atlas_catalog->next_slot;
    
    // NOTE: If the selected font atlas is recycled, we leak memory in the arena due to the
    // allocated array for the glyphs. We also leak memory due to the pixels allocation.
    // NOTE: The number of glyph may vary. We need to take care of this. Maybe a linked list of storage, which are recyled if enough space is available ?
    // TODO: Solve this
    
    *font_atlas = {};
    
    if (!create_font_atlas(font, font_atlas, font_size, first_unicode_character, character_count, arena)) {
        println("Error: failed to create font atlas");
        return false;
    }
    
    font_atlas->texture_array_index = font_atlas_catalog->next_slot;
    
    // Upload the texture to the GPU
    if (!copy_font_atlas(font_atlas, &font_atlas_catalog->resources, state)) {
        println("Error: failed to copy font atlas to the GPU.");
        return false;
    }
    
    font_atlas_catalog->next_slot = (font_atlas_catalog->next_slot + 1) % font_atlas_catalog->size;
    
    return true;
}

inline bool create_and_add_font_atlases(FontCatalog* font_catalog,
                                        ConstString* font_name,
                                        u32* font_sizes,
                                        u32 font_sizes_count,
                                        FontAtlasCatalog* font_atlas_catalog,
                                        u32 first_unicode_character,
                                        u32 character_count,
                                        RendererState* state,
                                        MemoryArena* arena) {
    for (u32 i = 0;i < font_sizes_count;++i) {
        bool result = create_and_add_font_atlas(font_catalog,
                                                font_name,
                                                font_sizes[i],
                                                font_atlas_catalog,
                                                first_unicode_character,
                                                character_count,
                                                state,
                                                arena);
        if (!result) {
            println("Error: failed to create font atlas for '%s@%d'", font_name->str, font_sizes[i]);
            return false;
        }
    }
    
    return true;
}

inline FontAtlas* get_font_atlas_from_catalog(FontAtlasCatalog* font_atlas_catalog,
                                              ConstString* font_name,
                                              u32 font_size) {
    for (u32 i = 0;i < font_atlas_catalog->size;++i) {
        FontAtlas* current = font_atlas_catalog->atlases + i;
        if (current->size == font_size && strcmp(font_name->str, current->font->name.str) == 0) {
            return current;
        }
    }
    
    return 0;
}

inline u32 get_ascii_character_index(FontAtlas* font_atlas, char character) {
    //println("char: %d with %d", character, font_atlas->first_unicode_character);
    assert((u32)character >= font_atlas->first_unicode_character);
    assert(((u32)character - font_atlas->first_unicode_character) < font_atlas->glyph_count);
    
    return (u32)character - font_atlas->first_unicode_character;
}

/*
inline char* to_string(FontAtlas* font_atlas, MemoryArena* temporary_storage, u32 indentation_level = 0, u32* bytes_written = 0) {
    char* indent_space = (char*)allocate(temporary_storage, indentation_level + 1);
    for (u32 i = 0;i < indentation_level;i++) {
        indent_space[i] = ' ';
    }
    indent_space[indentation_level] = 0;
    
    char* buffer = (char*)allocate(temporary_storage, 30000);
    
    sprintf(buffer,
            "%sFontAtlas {\n"
            "%s    pixels: %p\n"
            "%s    width: %d\n"
            "%s    height: %d\n"
            "%s    first_unicode_character: %d\n"
            "%s    glyph_count: %d\n"
            "%s    glyphs: [\n",
            indent_space,
            indent_space, font_atlas->pixels,
            indent_space, font_atlas->width,
            indent_space, font_atlas->height,
            indent_space, font_atlas->first_unicode_character,
            indent_space, font_atlas->glyph_count,
            indent_space);
    int header_bytes_written = strlen(buffer);
    
    char* new_buffer = buffer + header_bytes_written;
    for (u32 i = 0;i < font_atlas->glyph_count;++i) {
        u32 bytes_written = 0;
        sprintf(new_buffer, "%s,\n", to_string(&font_atlas->glyphs[i], temporary_storage, indentation_level + 8, &bytes_written));
        new_buffer += bytes_written + 2;
    }
    
    sprintf(new_buffer, "%s]", indent_space);
    
    if (bytes_written) {
        *bytes_written = strlen(buffer);
    }
    
    return buffer;
}

inline char* to_string(Glyph* glyph, MemoryArena* temporary_storage, u32 indentation_level = 0, u32* bytes_written = 0) {
    char* indent_space = (char*)allocate(temporary_storage, indentation_level + 1);
    for (u32 i = 0;i < indentation_level;i++) {
        indent_space[i] = ' ';
    }
    indent_space[indentation_level] = 0;
    
    char* buffer = (char*)allocate(temporary_storage, 200);
    
    sprintf(buffer,
            "%sGlyph {\n"
            "%s    right: %d\n"
            "%s    top: %d\n"
            "%s    left: %d\n"
            "%s    bottom: %d\n"
            "%s    x_offset: %f\n"
            "%s    y_offset: %f\n"
            "%s    x_advance: %f\n"
            "%s    x_offset_2: %f\n"
            "%s    y_offset_2: %f\n"
            "%s}",
            indent_space,
            indent_space, glyph->right,
            indent_space, glyph->top,
            indent_space, glyph->left,
            indent_space, glyph->bottom,
            indent_space, glyph->x_offset,
            indent_space, glyph->y_offset,
            indent_space, glyph->x_advance,
            indent_space, glyph->x_offset_2,
            indent_space, glyph->y_offset_2,
            indent_space);
    if (bytes_written != 0) {
        *bytes_written = strlen(buffer);
    }
    
    return buffer;
}

// NOTE: debug
inline void print_font_atlas_catalog(FontAtlasCatalog* font_atlas_catalog) {
    for (u32 i = 0;i < font_atlas_catalog->size;++i) {
        FontAtlas* font_atlas = font_atlas_catalog->atlases + i;
        println("    Font Atlas:\n"
                "        font name: %s\n"
                "        size: %d",
                font_atlas->font->name.str,
                font_atlas->size);
    }
}
*/