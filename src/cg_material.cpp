#include "cg_material.h"

inline bool init_material_catalog(MaterialCatalog* catalog, RendererState* state) {
    if (!init_memory_arena(&catalog->arena, MAX_MATERIAL_COUNT * MAX_MATERIAL_NAME_LENGTH * 2)) {
        return false;
    }
    
    catalog->materials = (Material*)zero_allocate(&state->main_arena,
                                                  MAX_MATERIAL_COUNT * sizeof(Material));
    if (catalog->materials == 0) {
        println("Error: failed to allocate material array.");
        return false;
    }
    
    catalog->material_entries = (MaterialEntry*)zero_allocate(&state->main_arena,
                                                              MAX_MATERIAL_COUNT * sizeof(MaterialEntry));
    if (catalog->material_entries == 0) {
        println("Error: failed to allocate material entry array.");
        return false;
    }
    
    if (!init_material_catalog_resources(&catalog->resources, state)) {
        println("Error: failed to init material catalog resources.");
        return false;
    }
    
    return true;
}

inline void destroy_material_catalog(MaterialCatalog* catalog, RendererState* state, bool verbose) {
    if (verbose) {
        println("Destroying Material Catalog");
    }
    
    destroy_material_catalog_resources(&catalog->resources, state, verbose);
    
    destroy_memory_arena(&catalog->arena, verbose);
}

inline static bool create_buffers(MaterialCatalogResources* material_catalog_resources, RendererState* state) {
    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = MAX_MATERIAL_COUNT * sizeof(Material);
    create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &state->selection.graphics_queue_family_index;
    
    VkResult result = vkCreateBuffer(state->device, &create_info, nullptr, &material_catalog_resources->buffer);
    if (result != VK_SUCCESS) {
        println("vkCreateBuffer returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    VkMemoryRequirements requirements = {};
    vkGetBufferMemoryRequirements(state->device, material_catalog_resources->buffer, &requirements);
    
    VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    
    if (!allocate(&state->memory_manager, state->device, requirements, memory_flags, &material_catalog_resources->allocation)) {
        println("Error: failed to allocate memory.");
        return false;
    }
    
    result = vkBindBufferMemory(state->device, material_catalog_resources->buffer, material_catalog_resources->allocation.device_memory, material_catalog_resources->allocation.offset);
    
    if (result != VK_SUCCESS) {
        println("vkBindBufferMemory returned (%s)",vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline bool init_material_catalog_resources(MaterialCatalogResources* material_catalog_resources, RendererState* state) {
    if (!create_buffers(material_catalog_resources, state)) {
        println("Error: failed to create buffers");
        return false;
    }
    
    return true;
}

inline void destroy_material_catalog_resources(MaterialCatalogResources* material_catalog_resources, RendererState* state, bool verbose) {
    if (material_catalog_resources->buffer) {
        if (verbose) {
            println("Destroying material buffer");
        }
        vkDestroyBuffer(state->device, material_catalog_resources->buffer, nullptr);
    }
    
    if (verbose) {
        free(&state->memory_manager, &material_catalog_resources->allocation);
    }
}

inline bool add_named_material(MaterialCatalog* catalog, Material new_material, ConstString material_name) {
    u32 added_id = 0;
    if (!add_material(catalog, new_material, &added_id)) {
        println("Error: failed to add new material.");
        return false;
    }
    
    MaterialEntry new_entry = {};
    new_entry.index = added_id;
    new_entry.material_name = push_string_copy(&catalog->arena, &material_name);
    
    bool success = insert_material_entry(catalog, new_entry);
    if (!success) {
        println("Error: failed to insert new material entry");
        return false;
    }
    
    return true;
}

inline bool add_material(MaterialCatalog* catalog, Material new_material, u32* material_index) {
    assert(catalog->material_count < MAX_MATERIAL_COUNT);
    
    *material_index = catalog->material_count++;
    
    catalog->materials[*material_index] = new_material;
    
    return true;
}

inline bool insert_material_entry(MaterialCatalog* catalog, MaterialEntry new_entry){
    if (catalog->entry_count == 0) {
        // This is the base case
        MaterialEntry* entry = catalog->material_entries;
        *entry = new_entry;
        catalog->entry_count++;
        return true;
    }
    
    u32 a = 0;
    u32 b = catalog->entry_count - 1;
    
    // Find the correct position in the sorted array
    while (a != b) {
        u32 m = (a + b) / 2;
        MaterialEntry* entry = catalog->material_entries + m;
        i32 result = string_compare(new_entry.material_name, entry->material_name);
        if (result > 0) {
            a = m + 1;
        } else if (result < 0) {
            b = m;
        } else {
            println("Error: material name already in use");
            return false;
        }
    }
    
    // Move the other entries
    for (u32 i = catalog->entry_count;i > a;--i) {
        catalog->material_entries[i] = catalog->material_entries[i - 1];
    }
    
    // Copy the new entry
    catalog->material_entries[a] = new_entry;
    catalog->entry_count++;
    
    return true;
}


inline i32 get_named_material_index(MaterialCatalog* catalog, ConstString material_name) {
    i32 index = -1;
    
    if (catalog->entry_count == 0) {
        // This is the base case
        return index;
    }
    
    u32 a = 0;
    u32 b = catalog->entry_count - 1;
    
    // we find the candidate position in the sorted array
    while (a != b) {
        u32 m = (a + b) / 2;
        MaterialEntry* entry = catalog->material_entries + m;
        i32 result = string_compare(material_name, entry->material_name);
        if (result > 0) {
            a = m + 1;
        } else if (result < 0) {
            b = m;
        } else {
            index = m;
            return index;
        }
    }
    
    // We check if it is the material we are looking for
    MaterialEntry* candidate = catalog->material_entries + a;
    if (string_compare(material_name, candidate->material_name) == 0) {
        index = a;
    }
    
    return index;
}

inline bool get_material_from_index(MaterialCatalog* catalog, u32 index, Material* material) {
    if (index < 0 || index > catalog->material_count) {
        return false;
    }
    
    *material = catalog->materials[index];
    return true;
}

inline bool get_named_material(MaterialCatalog* catalog, ConstString material_name, Material* material) {
    i32 index = get_named_material_index(catalog, material_name);
    if (index < 0) {
        return false;
    }
    
    return get_material_from_index(catalog, index, material);
}
