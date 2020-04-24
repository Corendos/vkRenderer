#include "memory.hpp"

#include <string.h>
#include <stdlib.h>

#include "macros.hpp"

bool init_memory(MemoryManager* manager, uint64_t allocation_size, uint64_t min_page_size, VkPhysicalDevice physical_device) {
    vkGetPhysicalDeviceMemoryProperties(physical_device, &manager->memory_properties);
    
    manager->pools = (MemoryPool**)calloc(manager->memory_properties.memoryTypeCount, sizeof(MemoryPool*));
    
    manager->allocation_size = allocation_size;
    manager->min_page_size = min_page_size;
    
    return true;
}

void cleanup_memory(MemoryManager* manager, VkDevice device, bool verbose = false) {
    if (manager == 0) return;
    for (int i = 0;i < manager->memory_properties.memoryTypeCount;++i) {
        if (verbose) {
            printf("Destroying allocation of type #%d\n", i);
        }
        MemoryPool* current = manager->pools[i];
        while (current != 0) {
            printf("    Freeing device memory (%p)\n", current->device_memory);
            cleanup_pool(manager, device, current);
            vkFreeMemory(device, current->device_memory, nullptr);
            MemoryPool* next = current->next;
            free_null(current);
            current = next;
        }
    }
    
    free_null(manager->pools);
}

void cleanup_pool(MemoryManager* manager, VkDevice device, MemoryPool* pool) {
    printf("    Cleaning pool (%p)\n", pool);
    
    if (pool->mappable) {
        vkUnmapMemory(device, pool->device_memory);
    }
    
    cleanup_chunk(manager, &pool->base_chunk);
}

void cleanup_chunk(MemoryManager* manager, MemoryChunk* chunk) {
    if (chunk->left) {
        cleanup_chunk(manager, chunk->left);
        free_null(chunk->left);
    }
    
    if (chunk->right) {
        cleanup_chunk(manager, chunk->right);
        free_null(chunk->right);
    }
}

int32_t find_memory_type_index(MemoryManager* manager, VkMemoryRequirements requirements, VkMemoryPropertyFlags required_properties) {
    for (int i = 0;i < manager->memory_properties.memoryTypeCount;++i) {
        uint32_t memory_type_bit = (1 << i);
        bool valid_memory_type = requirements.memoryTypeBits & memory_type_bit;
        
        bool valid_flags = (required_properties & manager->memory_properties.memoryTypes[i].propertyFlags) == required_properties;
        
        if (valid_memory_type && valid_flags) return i;
    }
    
    return -1;
}

bool allocate_pool_for_type(MemoryManager* manager, VkDevice device, uint32_t type, VkMemoryPropertyFlags flags, MemoryPool** pool) {
    MemoryPool* new_pool = (MemoryPool*)calloc(1, sizeof(MemoryPool));
    new_pool->base_chunk.size = manager->allocation_size;
    new_pool->memory_type = type;
    
    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = manager->allocation_size;
    allocate_info.memoryTypeIndex = type;
    
    VkResult result = vkAllocateMemory(device, &allocate_info, nullptr, &new_pool->device_memory);
    
    if (result != VK_SUCCESS) {
        free_null(new_pool);
        return false;
    }
    
    if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        new_pool->mappable = true;
        VkResult result = vkMapMemory(device, new_pool->device_memory, 0, VK_WHOLE_SIZE, 0, &new_pool->data);
        if (result != VK_SUCCESS) {
            vkFreeMemory(device, new_pool->device_memory, nullptr);
            free_null(new_pool);
            return false;
        }
    }
    
    *pool = new_pool;
    
    return true;
}

bool allocate(MemoryManager* manager, VkDevice device, VkMemoryRequirements requirements, VkMemoryPropertyFlags required_properties, AllocatedMemoryChunk* allocated_chunk) {
    if (requirements.size > manager->allocation_size) return false; // We will never find a spot big enough
    
    int32_t memory_type = find_memory_type_index(manager, requirements, required_properties);
    if (memory_type == -1) return false;
    
    MemoryPool* pool = manager->pools[memory_type];
    if (!pool) {
        MemoryPool* just_allocated_pool = 0;
        if(!allocate_pool_for_type(manager, device, memory_type, required_properties, &just_allocated_pool)) {
            return false;
        }
        
        manager->pools[memory_type] = just_allocated_pool;
        
        return allocate_from_pool(manager, just_allocated_pool, requirements, allocated_chunk);
    }
    
    MemoryPool* last_pool = 0;
    while (pool) {
        if (allocate_from_pool(manager, pool, requirements, allocated_chunk)) return true;
        last_pool = pool;
        pool = pool->next;
    }
    
    MemoryPool* just_allocated_pool = 0;
    if(!allocate_pool_for_type(manager, device, memory_type, required_properties, &just_allocated_pool)) {
        return false;
    }
    
    last_pool->next = just_allocated_pool;
    
    return allocate_from_pool(manager, just_allocated_pool, requirements, allocated_chunk);
}

bool allocate_from_pool(MemoryManager* manager, MemoryPool* pool, VkMemoryRequirements requirements, AllocatedMemoryChunk* allocated_chunk) {
    if (pool == 0) return false;
    if (requirements.size > manager->allocation_size) return false;
    
    return allocate_from_chunk(manager, pool, &pool->base_chunk, requirements, allocated_chunk);
}

bool allocate_from_chunk(MemoryManager* manager, MemoryPool* pool, MemoryChunk* chunk, VkMemoryRequirements requirements, AllocatedMemoryChunk* allocated_chunk) {
    if (chunk->full) {
        return false;
    }
    if (requirements.size > chunk->size) return false; // This should never happen
    
    if (chunk->size == manager->min_page_size) {
        allocated_chunk->device_memory  = pool->device_memory;
        allocated_chunk->memory_type    = pool->memory_type;
        allocated_chunk->allocated_size = chunk->size;
        allocated_chunk->real_size      = requirements.size;
        allocated_chunk->offset         = chunk->offset;
        allocated_chunk->mappable       = pool->mappable;
        allocated_chunk->data           = (uint8_t*)pool->data + chunk->offset;
        
        chunk->full = true;
        return true;
    }
    
    VkDeviceSize sub_chunk_size = chunk->size / 2;
    if (requirements.size > sub_chunk_size) {
        if (!chunk->left && !chunk->right) {
            allocated_chunk->device_memory  = pool->device_memory;
            allocated_chunk->memory_type    = pool->memory_type;
            allocated_chunk->allocated_size = chunk->size;
            allocated_chunk->real_size      = requirements.size;
            allocated_chunk->offset         = chunk->offset;
            allocated_chunk->mappable       = pool->mappable;
            allocated_chunk->data           = (uint8_t*)pool->data + chunk->offset;
            chunk->full = true;
            return true;
        }
        
        return false;
    }
    
    if (chunk->left == 0 && chunk->right == 0) {
        MemoryChunk* left  = (MemoryChunk*)calloc(1, sizeof(MemoryChunk));
        left->offset = chunk->offset;
        left->size   = sub_chunk_size;
        
        MemoryChunk* right = (MemoryChunk*)calloc(1, sizeof(MemoryChunk));
        right->offset = chunk->offset + sub_chunk_size;
        right->size   = sub_chunk_size;
        
        chunk->left = left;
        chunk->right = right;
        return allocate_from_chunk(manager, pool, chunk->left, requirements, allocated_chunk); // This should never fail because we created a chunk big enough
    }
    
    bool found_left = allocate_from_chunk(manager, pool, chunk->left, requirements, allocated_chunk);
    if (found_left) {
        chunk->full = chunk->left->full && chunk->right->full;
        
        return true;
    }
    
    bool found_right = allocate_from_chunk(manager, pool, chunk->right, requirements, allocated_chunk);
    if (found_right) {
        chunk->full = chunk->left->full && chunk->right->full;
        
        return true;
    }
    
    // @Note: maybe we should assert that when we come to this portion we are not in the case where the children have just been created ?
    
    return false; // In the case where two children tree already exist and we've not found a suitable location.
}

void free(MemoryManager* manager, AllocatedMemoryChunk* allocated_chunk) {
    if (!allocated_chunk) return;
    if (!allocated_chunk->device_memory) return;
    
    MemoryPool* pool = manager->pools[allocated_chunk->memory_type];
    while (pool) {
        if (pool->device_memory == allocated_chunk->device_memory) {
            free_from_pool(manager, pool, allocated_chunk);
            return;
        }
        pool = pool->next;
    }
}

void free_from_pool(MemoryManager* manager, MemoryPool* pool, AllocatedMemoryChunk* allocated_chunk) {
    if (allocated_chunk->device_memory != pool->device_memory) return;
    
    free_from_chunk(manager, pool, &pool->base_chunk, allocated_chunk);
}

void free_from_chunk(MemoryManager* manager, MemoryPool* pool, MemoryChunk* chunk, AllocatedMemoryChunk* allocated_chunk) {
    if (allocated_chunk->allocated_size == chunk->size && allocated_chunk->offset == chunk->offset) {
        chunk->full = false;
        return;
    }
    
    uint32_t sub_chunk_size = chunk->size / 2;
    if (allocated_chunk->offset >= chunk->offset + sub_chunk_size) {
        if (chunk->right == 0) {
            printf("**** ERROR ****\n");
            // Error this should not happen. This means that this chunk has not been split but we have an allocated subchunk
            return;
        }
        free_from_chunk(manager, pool, chunk->right, allocated_chunk);
    } else {
        if (chunk->left == 0) {
            printf("**** ERROR ****\n");
            // Error this should not happen. This means that this chunk has not been split but we have an allocated subchunk
            return;
        }
        free_from_chunk(manager, pool, chunk->left, allocated_chunk);
    }
    
    if (!chunk->left->left && !chunk->left->right && !chunk->right->left && !chunk->right->right && !chunk->left->full && !chunk->right->full) {
        free_null(chunk->left);
        free_null(chunk->right);
    }
    
    chunk->full = false;
    return;
}

void memory_snapshot(MemoryManager* manager, MemoryPool* pool, uint8_t* occupancy) {
    memory_snapshot_chunk(manager, &pool->base_chunk, occupancy);
}

void memory_snapshot_chunk(MemoryManager* manager, MemoryChunk* chunk, uint8_t* occupancy) {
    if (chunk->full) {
        uint32_t start_index = chunk->offset / manager->min_page_size;
        uint32_t size = chunk->size / manager->min_page_size;
        for (int i = start_index;i < start_index + size;++i) {
            occupancy[i] = 1;
        }
        return;
    }
    
    if (chunk->left) {
        memory_snapshot_chunk(manager, chunk->left, occupancy);
    }
    
    if (chunk->right) {
        memory_snapshot_chunk(manager, chunk->right, occupancy);
    }
}
