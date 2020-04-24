#ifndef __SHADERS_HPP__
#define __SHADERS_HPP__

#include <stdint.h>

#include <vulkan/vulkan.h>

struct ShaderCatalog {
    VkShaderModule* modules;
    uint32_t count;
};

bool load_shader_code(const char* filename, uint32_t** code, uint32_t* code_size);
void free_shader_code(uint32_t* code);

bool create_shader_module(VkDevice device, const char* filename, VkShaderModule* module);

bool load_shader_module(const char* filename, const char* shader_name, VkDevice device, ShaderCatalog* catalog);
bool get_shader_from_catalog(const char* shader_name, ShaderCatalog* catalog, VkShaderModule* module);
bool init_shader_catalog(uint32_t size, ShaderCatalog* catalog);
void cleanup_shader_catalog(VkDevice device, ShaderCatalog* catalog, bool verbose);


#endif
