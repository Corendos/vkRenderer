#ifndef __SHADERS_H__
#define __SHADERS_H__

#include <stdint.h>

#include <vulkan/vulkan.h>

struct ShaderCatalog {
    VkShaderModule* modules;
    u32 count;
};

bool init_shader_catalog(ShaderCatalog* catalog, u32 size);
void cleanup_shader_catalog(VkDevice device, ShaderCatalog* catalog, bool verbose);

bool load_shader_code(const char* filename, u32** code, u32* code_size);
void free_shader_code(u32* code);

bool create_shader_module(VkDevice device, const char* filename, VkShaderModule* module);

bool load_shader_module(const char* filename, const char* shader_name, VkDevice device, ShaderCatalog* catalog);
bool get_shader_from_catalog(const char* shader_name, ShaderCatalog* catalog, VkShaderModule* module);


#endif
