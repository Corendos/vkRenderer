#include "shaders.hpp"

#include <assert.h>

#include "hash.hpp"
#include "macros.hpp"
#include "vk_helper.hpp"
#include "utils.hpp"

u32 get_file_size(FILE* file) {
    u32 size = 0;
    
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    return size;
}

void copy_file_to(FILE* file, char* dest, u32 file_size = 0) {
    if (file_size == 0) {
        file_size = get_file_size(file);
    }
    
    char* pos = dest;
    for (int i = 0;i < file_size;++i) {
        *pos++ = fgetc(file);
    }
}

bool load_shader_code(const char* filename, u32** code, u32* code_size) {
    char full_filename[256] = {0};
    // @Warning: this is unchecked
    get_full_path_from_root(filename, full_filename);
    
    FILE* file = fopen(full_filename, "rb");
    
    if (!file) {
        return false;
    }
    
    *code_size = get_file_size(file);
    
    char* char_code = (char*)malloc(*code_size);
    
    copy_file_to(file, char_code, *code_size);
    
    *code = (u32*)char_code;
    
    fclose(file);
    
    return true;
}

void free_shader_code(u32* code) {
    free_null(code);
}

bool create_shader_module(VkDevice device, const char* filename, VkShaderModule* module) {
    u32  code_size = 0;
    u32* code = 0;
    
    if (!load_shader_code(filename, &code, &code_size)) {
        free_null(code);
        return false;
    }
    
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code_size;
    create_info.pCode = code;
    
    VkResult result = vkCreateShaderModule(device, &create_info, nullptr, module);
    if (result != VK_SUCCESS) {
        printf("vkCreateShaderModule returned (%s)\n", vk_error_code_str(result));
        free_null(code);
        return false;
    }
    
    free_null(code);
    return true;
}

bool load_shader_module(const char* filename, const char* shader_name, VkDevice device, ShaderCatalog* catalog) {
    if (catalog->count <= 0) {
        printf("Error: catalog is not allocated.\n");
        return false;
    }
    
    u32 index = hash(shader_name) % catalog->count;
    
    if (catalog->modules[index] != 0) {
        printf("Error: shader name collision in hashtable.\n");
        return false;
    }
    
    if (!create_shader_module(device, filename, &catalog->modules[index])) {
        printf("Error: failed to load shader code.\n");
        return false;
    }
    
    return true;
}

bool get_shader_from_catalog(const char* shader_name, ShaderCatalog* catalog, VkShaderModule* module) {
    if (catalog->count <= 0) {
        printf("Error: catalog is not allocated.\n");
        return false;
    }
    
    u32 index = hash(shader_name) % catalog->count;
    
    if (catalog->modules[index] == 0) {
        printf("Error: shader name has not been loaded.\n");
        return false;
    }
    
    *module = catalog->modules[index];
    
    return true;
}

bool init_shader_catalog(u32 size, ShaderCatalog* catalog) {
    catalog->modules = (VkShaderModule*)calloc(size, sizeof(VkShaderModule));
    catalog->count = size;
    
    return true;
}

void cleanup_shader_catalog(VkDevice device, ShaderCatalog* catalog, bool verbose = false) {
    if (catalog->modules) {
        if (verbose) {
            printf("Destroying shader catalog\n");
        }
        
        for (int i = 0;i < catalog->count;++i) {
            if (catalog->modules[i]) {
                if (verbose) {
                    printf("Destroying shader module (%p)\n", catalog->modules[i]);
                }
                vkDestroyShaderModule(device, catalog->modules[i], nullptr);
            }
        }
        
        free_null(catalog->modules);
    }
}
