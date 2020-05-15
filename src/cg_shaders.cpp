#include "cg_shaders.h"

#include <assert.h>

#include "cg_hash.h"
#include "cg_macros.h"
#include "cg_vk_helper.h"
#include "cg_utils.h"

inline bool load_shader_code(const char* filename, u32** code, u32* code_size) {
    char full_filename[256] = {0};
    // @Warning: this is unchecked
    get_full_path_from_root(filename, full_filename);
    
    FILE* file = fopen(full_filename, "rb");
    
    if (!file) {
        return false;
    }
    
    *code_size = get_file_size(file);
    
    u8* char_code = (u8*)malloc(*code_size);
    
    copy_file_to(file, char_code, *code_size);
    
    *code = (u32*)char_code;
    
    fclose(file);
    
    return true;
}

inline void free_shader_code(u32* code) {
    free_null(code);
}

inline bool create_shader_module(VkDevice device, const char* filename, VkShaderModule* module) {
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
        println("vkCreateShaderModule returned (%s)", vk_error_code_str(result));
        free_null(code);
        return false;
    }
    
    free_null(code);
    return true;
}

inline bool load_shader_module(const char* filename, const char* shader_name, VkDevice device, ShaderCatalog* catalog) {
    if (catalog->count <= 0) {
        println("Error: catalog is not allocated.");
        return false;
    }
    
    u32 index = hash(shader_name) % catalog->count;
    
    if (catalog->modules[index] != 0) {
        println("Error: shader name collision in hashtable.");
        return false;
    }
    
    if (!create_shader_module(device, filename, &catalog->modules[index])) {
        println("Error: failed to load shader code.");
        return false;
    }
    
    return true;
}

inline bool get_shader_from_catalog(const char* shader_name, ShaderCatalog* catalog, VkShaderModule* module) {
    if (catalog->count <= 0) {
        println("Error: catalog is not allocated.");
        return false;
    }
    
    u32 index = hash(shader_name) % catalog->count;
    
    if (catalog->modules[index] == 0) {
        println("Error: shader name has not been loaded.");
        return false;
    }
    
    *module = catalog->modules[index];
    
    return true;
}

inline bool init_shader_catalog(ShaderCatalog* catalog, u32 size) {
    catalog->modules = (VkShaderModule*)calloc(size, sizeof(VkShaderModule));
    catalog->count = size;
    
    return true;
}

inline void cleanup_shader_catalog(VkDevice device, ShaderCatalog* catalog, bool verbose = false) {
    if (catalog->modules) {
        if (verbose) {
            println("Destroying shader catalog");
        }
        
        for (int i = 0;i < catalog->count;++i) {
            if (catalog->modules[i]) {
                if (verbose) {
                    println("Destroying shader module (%p)", catalog->modules[i]);
                }
                vkDestroyShaderModule(device, catalog->modules[i], nullptr);
            }
        }
        
        free_null(catalog->modules);
    }
}
