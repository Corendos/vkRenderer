#ifndef __CG_MATERIAL_H__
#define __CG_MATERIAL_H__

#include <vulkan/vulkan.h>
#include "cg_math.h"
#include "cg_string.h"
#include "cg_memory.h"

#define MAX_MATERIAL_COUNT 64
#define MAX_MATERIAL_NAME_LENGTH 128

struct RendererState;

struct Material {
    Vec3f ambient_color;
    f32 _dummy1;
    Vec3f diffuse_color;
    f32 _dummy2;
    Vec3f specular_color;
    f32   specular_exponent;
};

struct MaterialEntry {
    ConstString material_name;
    u32 index;
};

struct MaterialCatalogResources {
    VkBuffer buffer;
    AllocatedMemoryChunk allocation;
    VkDescriptorSet* descriptor_sets;
};

struct MaterialCatalog {
    Material* materials;
    u32 material_count;
    
    MaterialEntry* material_entries;
    u32 entry_count;
    MaterialCatalogResources resources;
    MemoryArena arena;
};

bool init_material_catalog(MaterialCatalog* catalog, RendererState* state);
void destroy_material_catalog(MaterialCatalog* catalog, RendererState* state, bool verbose = false);

bool init_material_catalog_resources(MaterialCatalogResources* material_catalog_resources, RendererState* state);
void destroy_material_catalog_resources(MaterialCatalogResources* material_catalog_resources, RendererState* state, bool verbose = false);

bool add_named_material(MaterialCatalog* catalog, Material new_material, ConstString material_name);
bool add_material(MaterialCatalog* catalog, Material new_material, u32* material_index);
bool insert_material_entry(MaterialCatalog* catalog, MaterialEntry new_entry);

i32 get_named_material_index(MaterialCatalog* catalog, ConstString material_name);
bool get_material_from_index(MaterialCatalog* catalog, u32 index, Material* material);
bool get_named_material(MaterialCatalog* catalog, ConstString material_name, Material* material);

#endif //CG_MATERIAL_H
