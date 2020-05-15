#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <vulkan/vulkan.h>

struct RendererState;

struct AllocatedMemoryChunk {
    VkDeviceMemory device_memory;
    u32 memory_type;
    
    VkDeviceSize offset;
    VkDeviceSize allocated_size;
    VkDeviceSize real_size;
    
    bool mappable;
    void* data;
};

struct MemoryChunk {
    VkDeviceSize offset;
    VkDeviceSize size;
    
    bool full;
    
    MemoryChunk* right;
    MemoryChunk* left;
};

struct MemoryPool {
    VkDeviceMemory device_memory;
    u32 memory_type;
    
    MemoryChunk base_chunk;
    
    bool mappable;
    void* data;
    MemoryPool* next;
};

struct MemoryManager {
    MemoryPool** pools;
    
    VkDeviceSize allocation_size;
    VkDeviceSize min_page_size;
    
    VkPhysicalDeviceMemoryProperties memory_properties;
};


bool init_memory(MemoryManager* manager, u64 allocation_size, u64 min_page_size, VkPhysicalDevice physical_device);
void cleanup_memory(MemoryManager* manager, VkDevice device, bool verbose);
void cleanup_chunk(MemoryManager* manager, MemoryChunk* chunk);
void cleanup_pool(MemoryManager* manager, VkDevice device, MemoryPool* pool);

i32 find_memory_type_index(MemoryManager* manager, VkMemoryRequirements requirements, VkMemoryPropertyFlags required_properties);

bool allocate_pool_for_type(MemoryManager* manager, VkDevice device, u32 type, VkMemoryPropertyFlags flags, MemoryPool* pool);

bool allocate(MemoryManager* manager, VkDevice device, VkMemoryRequirements requirements, VkMemoryPropertyFlags required_properties, AllocatedMemoryChunk* allocated_chunk);
bool allocate_from_pool(MemoryManager* manager, MemoryPool* pool, VkMemoryRequirements requirements, AllocatedMemoryChunk* allocated_chunk);
bool allocate_from_chunk(MemoryManager* manager, MemoryPool* pool, MemoryChunk* chunk, VkMemoryRequirements requirements, AllocatedMemoryChunk* allocated_chunk);

void free(MemoryManager* manager, AllocatedMemoryChunk* allocated_chunk);
void free_from_chunk(MemoryManager* manager, MemoryPool* pool, MemoryChunk* chunk, AllocatedMemoryChunk* allocated_chunk);
void free_from_pool(MemoryManager* manager, MemoryPool* pool, AllocatedMemoryChunk* allocated_chunk);

void memory_snapshot(MemoryManager* manager, MemoryPool* pool, u8* occupancy);
void memory_snapshot_chunk(MemoryManager* manager, MemoryChunk* chunk, u8* occupancy);

#endif
