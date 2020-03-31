#include "shaders.hpp"

#include <iostream>
#include <cassert>
#include <cstring>

#include "hash.hpp"
#include "macros.hpp"
#include "vk_helper.hpp"
#include "utils.hpp"

uint32_t get_file_size(FILE* file) {
    uint32_t size = 0;

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);

    return size;
}

void copy_file_to(FILE* file, char* dest, uint32_t file_size = 0) {
    if (file_size == 0) {
	file_size = get_file_size(file);
    }

    char* pos = dest;
    for (int i = 0;i < file_size;++i) {
	*pos++ = fgetc(file);
    }
}

bool load_shader_code(const char* filename, uint32_t** code, uint32_t* code_size) {
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

    *code = (uint32_t*)char_code;

    fclose(file);

    return true;
}

void free_shader_code(uint32_t* code) {
    free_null(code);
}

bool create_shader_module(VkDevice device, const char* filename, VkShaderModule* module) {
    uint32_t  code_size = 0;
    uint32_t* code = 0;

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
	std::cout << "vkCreateShaderModule returned (" << vk_error_code_str(result) << ")" << std::endl;
	free_null(code);
	return false;
    }

    free_null(code);
    return true;
}

bool load_shader_module(const char* filename, const char* shader_name, VkDevice device, ShaderCatalog* catalog) {
    if (catalog->count <= 0) {
	std::cout << "Error: catalog is not allocated." << std::endl;
	return false;
    }

    uint32_t index = hash(shader_name) % catalog->count;

    if (catalog->modules[index] != 0) {
	std::cout << "Error: shader name collision in hashtable." << std::endl;
	return false;
    }

    if (!create_shader_module(device, filename, &catalog->modules[index])) {
	std::cout << "Error: failed to load shader code" << std::endl;
	return false;
    }

    return true;
}

bool get_shader_from_catalog(const char* shader_name, ShaderCatalog* catalog, VkShaderModule* module) {
    if (catalog->count <= 0) {
	std::cout << "Error: catalog is not allocated." << std::endl;
	return false;
    }

    uint32_t index = hash(shader_name) % catalog->count;

    if (catalog->modules[index] == 0) {
	std::cout << "Error: shader name has not been loaded." << std::endl;
	return false;
    }

    *module = catalog->modules[index];

    return true;
}

bool init_shader_catalog(uint32_t size, ShaderCatalog* catalog) {
    catalog->modules = (VkShaderModule*)calloc(size, sizeof(VkShaderModule));
    catalog->count = size;

    return true;
}

void cleanup_shader_catalog(VkDevice device, ShaderCatalog* catalog, bool verbose = false) {
    if (catalog->modules) {
	if (verbose) {
	    std::cout << "Destroying shader catalog" << std::endl;
	}

	for (int i = 0;i < catalog->count;++i) {
	    if (catalog->modules[i]) {
		if (verbose) {
		    std::cout << "Destroying shader module (" << catalog->modules[i] << ")" << std::endl;
		}
		vkDestroyShaderModule(device, catalog->modules[i], nullptr);
	    }
	}

	free_null(catalog->modules);
    }
}
