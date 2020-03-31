#include "texture.hpp"

#include <iostream>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "hash.hpp"
#include "macros.hpp"
#include "renderer.hpp"
#include "vk_helper.hpp"
#include "utils.hpp"

bool create_texture_catalog_staging_buffer(RendererState* state) {
    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = 4096 * 4096 * 4; // Enough space to store a 4096 by 4096 RGBA8 texture
    create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &state->selection.graphics_queue_family_index;

    VkResult result = vkCreateBuffer(state->device, &create_info, nullptr, &state->texture_catalog.staging_buffer);

    if (result != VK_SUCCESS) {
	std::cout << "vkCreateBuffer returned (" << vk_error_code_str(result) << ")" << std::endl;
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
	std::cout << "vkBindBufferMemory returned (" << vk_error_code_str(result) << ")" << std::endl;
	return false;
    }

    return true;
}

bool create_texture_catalog_command_pool(RendererState* state) {
    VkCommandPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    create_info.queueFamilyIndex = state->selection.graphics_queue_family_index;

    VkResult result = vkCreateCommandPool(state->device, &create_info, nullptr, &state->texture_catalog.command_pool);
    if (result != VK_SUCCESS) {
	std::cout << "vkCreateCommandPool returned (" << vk_error_code_str(result) << ")" << std::endl;
	return false;
    }

    return true;
}

bool init_texture_catalog(RendererState* state, uint32_t size) {
    TextureCatalog* catalog = &state->texture_catalog;
    catalog->textures = (Texture*)calloc(size, sizeof(Texture));
    catalog->count = size;

    if (!create_texture_catalog_staging_buffer(state)) {
	std::cout << "Error: failed to create staging buffer" << std::endl;
	return false;
    }

    if (!create_texture_catalog_command_pool(state)) {
	std::cout << "Error: failed to create command pool" << std::endl;
	return false;
    }

    return true;
}

void destroy_texture_catalog_staging_buffer(RendererState* state, bool verbose) {
    if (state->texture_catalog.staging_buffer) {
	if (verbose) {
	    std::cout << "Destroying texture catalog staging buffer (" << state->texture_catalog.staging_buffer << ")" << std::endl;
	}
	vkDestroyBuffer(state->device, state->texture_catalog.staging_buffer, nullptr);
    }
}

void destroy_texture_catalog_command_pool(RendererState* state, bool verbose) {
    if (state->texture_catalog.command_pool) {
	if (verbose) {
	    std::cout << "Destroying texture catalog command pool (" << state->texture_catalog.command_pool << ")" << std::endl;
	}
	vkDestroyCommandPool(state->device, state->texture_catalog.command_pool, nullptr);
    }
}

void destroy_textures(RendererState* state, bool verbose) {
    TextureCatalog* catalog = &state->texture_catalog;
    if (catalog->textures) {
	for (int i = 0;i < catalog->count;++i) {
	    if (catalog->textures[i].image) {
		if (verbose) {
		    std::cout << "Destroying image (" << catalog->textures[i].image << ")" << std::endl;
		}
		vkDestroyImage(state->device, catalog->textures[i].image, nullptr);

		free(&state->memory_manager, &catalog->textures[i].allocation);
	    }
	}

	free_null(catalog->textures);
    }
}

void cleanup_texture_catalog(RendererState* state, bool verbose) {
    // @Note: don't forget to destroy image handle eventually
    destroy_texture_catalog_staging_buffer(state, verbose);
    destroy_texture_catalog_command_pool(state, verbose);
    destroy_textures(state, verbose);
}

VkFormat get_format_from_channels(uint32_t channels) {
    switch (channels) {
      case 1:
	return VK_FORMAT_R8_UNORM;
      case 2:
	return VK_FORMAT_R8G8_UNORM;
      case 3:
	return VK_FORMAT_B8G8R8_UNORM;
      case 4:
	return VK_FORMAT_B8G8R8A8_UNORM;
    }
    return VK_FORMAT_UNDEFINED;
}

bool create_texture(RendererState* state, Texture* texture, uint32_t width, uint32_t height, uint32_t channels) {
    texture->width    = width;
    texture->height   = height;
    texture->channels = channels;

    VkImageCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.imageType = VK_IMAGE_TYPE_2D;
    create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    create_info.extent = { width, height, 1 };
    create_info.mipLevels = 1;
    create_info.arrayLayers = 1;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &state->selection.graphics_queue_family_index;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult result = vkCreateImage(state->device, &create_info, nullptr, &texture->image);
    if (result != VK_SUCCESS) {
	std::cout << "vkCreateImage returned (" << vk_error_code_str(result) << ")" << std::endl;
	return false;
    }

    VkMemoryRequirements requirements = {};

    vkGetImageMemoryRequirements(state->device, texture->image, &requirements);

    VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if(!allocate(&state->memory_manager, state->device, requirements, memory_flags, &texture->allocation)) {
	std::cout << "Error: failed to allocate for image" << std::endl;
	return false;
    }

    result = vkBindImageMemory(state->device, texture->image, texture->allocation.device_memory, texture->allocation.offset);
    if (result != VK_SUCCESS) {
	std::cout << "vkBindImageMemory returned (" << vk_error_code_str(result) << ")" << std::endl;
	return false;
    }

    return true;
}

bool copy_from_staging_buffer(RendererState* state, Texture* texture) {
    TextureCatalog* catalog = &state->texture_catalog;

    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = catalog->command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer = {};

    VkResult result = vkAllocateCommandBuffers(state->device, &allocate_info, &command_buffer);
    if (result != VK_SUCCESS) {
	std::cout << "vkAllocateCommandBuffers returned (" << result << ")" << std::endl;
	return false;
    }

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(command_buffer, &begin_info);
    if (result != VK_SUCCESS) {
	std::cout << "vkBeginCommandBuffer returned (" << result << ")" << std::endl;
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
	std::cout << "vkEndCommandBuffer returned (" << result << ")" << std::endl;
	return false;
    }

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    result = vkQueueSubmit(state->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    if (result != VK_SUCCESS) {
	std::cout << "vkQueueSubmit returned (" << result << ")" << std::endl;
	return false;
    }

    result = vkQueueWaitIdle(state->graphics_queue);
    if (result != VK_SUCCESS) {
	std::cout << "vkQueueWaitIdle returned (" << result << ")" << std::endl;
	return false;
    }

    vkFreeCommandBuffers(state->device, catalog->command_pool, 1, &command_buffer);

    return true;
}

bool load_texture(RendererState *state, const char* filename, const char* texture_name) {
    TextureCatalog* catalog = &state->texture_catalog;
    if (catalog->count <= 0) {
	std::cout << "Error: catalog is not allocated." << std::endl;
	return false;
    }

    uint32_t index = hash(texture_name) % catalog->count;

    if (catalog->textures[index].image != 0) {
	std::cout << "Error: texture name collision in hashtable." << std::endl;
	return false;
    }

    char full_filename[256] = {0};
    // @Warning: this is unchecked
    get_full_path_from_root(filename, full_filename);

    int width, height, channels;
    uint8_t* pixels = stbi_load(full_filename, &width, &height, &channels, 4);
    if (!pixels) {
	std::cout << "Error: failed to load image '" << filename << "'." << std::endl;
	return false;
    }

    // Copy the data to the staging buffer
    std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds> start = std::chrono::high_resolution_clock::now();
    memcpy(catalog->allocation.data, pixels, width * height * 4);
    std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds> end = std::chrono::high_resolution_clock::now();

    std::cout << "memcpy took " << (end - start).count() << "ns" << std::endl;

    stbi_image_free(pixels);

    if (!create_texture(state, &catalog->textures[index], width, height, 4)) {
	std::cout << "Error: failed to create texture" << std::endl;
	return false;
    }

    if(!copy_from_staging_buffer(state, &catalog->textures[index])) {
	std::cout << "Error: failed to copy from staging buffer" << std::endl;
	return false;
    }

    return true;
}
