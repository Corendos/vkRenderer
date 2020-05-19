#ifndef __CG_FONTS_H__
#define __CG_FONTS_H__

#include <vulkan/vulkan.h>

#include "stb_rect_pack.h"
#include "stb_truetype.h"
#include "cg_memory_arena.h"
#include "cg_texture.h"
#include "cg_string.h"

#define FONT_ATLAS_SIZE 512

struct Font {
    stbtt_fontinfo font_info;
    ConstString name;
};

struct FontEntry {
    Font font;
    u64 hash;
    FontEntry* next;
};

struct FontCatalog {
    FontEntry** entries;
    u32 size;
};

struct Glyph {
    u16 left;
    u16 top;
    u16 right;
    u16 bottom;
    i32 x_offset;
    i32 y_offset;
    i32 x_advance;
    i32 x_offset_2;
    i32 y_offset_2;
};

struct FontAtlas {
    u8* pixels;
    u32 width;
    u32 height;
    
    u32 size;
    
    Font* font;
    
    u32 first_unicode_character;
    
    Glyph* glyphs;
    u32 glyph_count;
    
    u32 texture_array_index;
};

struct FontAtlasCatalogResources {
    VkImage texture_array;
    VkImageView texture_array_image_view;
    VkSampler texture_array_sampler;
    VkBuffer staging_buffer;
    VkCommandPool command_pool;
    VkFence transfer_fence;
    VkDescriptorSet descriptor_set;
    AllocatedMemoryChunk texture_array_allocation;
    AllocatedMemoryChunk staging_buffer_allocation;
};

struct FontAtlasCatalog {
    //FontAtlasEntry** entries;
    u32 size;
    u32 next_slot;
    FontAtlas* atlases;
    FontAtlasCatalogResources resources;
};


u8* load_font_data_from_file(ConstString* filename, MemoryArena* storage);
bool load_font_from_data(Font* font, ConstString* font_name, const u8* font_data, MemoryArena* storage);
bool load_font_from_file(Font* font, ConstString* font_name, ConstString* filename, MemoryArena* storage);

// Font Catalog //
bool init_font_catalog(FontCatalog* catalog, u32 size, MemoryArena* storage);

bool add_font_to_catalog(FontCatalog* catalog, Font* font, MemoryArena* storage);
bool load_and_add_font_to_catalog(FontCatalog* catalog, ConstString* font_name, ConstString* filename, MemoryArena* storage);
Font* get_font_from_catalog(FontCatalog* catalog, ConstString* font_name);

u64 get_font_hash(ConstString* font_name, u32 size);


// Font Atlas Catalog //
bool init_font_atlas_catalog_resources(FontAtlasCatalog* catalog, RendererState* state, MemoryArena* storage);
bool init_font_atlas_catalog(FontAtlasCatalog* catalog, u32 size, RendererState* state, MemoryArena* storage);
void destroy_font_atlas_catalog(RendererState* state, FontAtlasCatalog* catalog, bool verbose);

bool create_font_atlas(Font* font,
                       FontAtlas* font_atlas,
                       u32 font_size,
                       u32 first_unicode_character,
                       u32 character_count,
                       MemoryArena* storage);
bool copy_font_atlas(FontAtlas* font_atlas,
                     FontAtlasCatalogResources* resources,
                     RendererState* state);
bool create_and_add_font_atlas(FontCatalog* font_catalog,
                               ConstString* font_name,
                               u32 font_size,
                               FontAtlasCatalog* font_atlas_catalog,
                               u32 first_unicode_character,
                               u32 character_count,
                               RendererState* state,
                               MemoryArena* arena);
bool create_and_add_font_atlases(FontCatalog* font_catalog,
                                 const char* font_name,
                                 u32 font_name_length,
                                 u32* font_sizes,
                                 u32 font_sizes_count,
                                 FontAtlasCatalog* font_atlas_catalog,
                                 u32 first_unicode_character,
                                 u32 character_count,
                                 RendererState* state,
                                 MemoryArena* arena);

FontAtlas* get_font_atlas_from_catalog(FontAtlasCatalog* font_atlas_catalog,
                                       ConstString* font_name,
                                       u32 font_size);


u32 get_ascii_character_index(FontAtlas* font_atlas, char character);

char* to_string(FontAtlas* font_atlas,
                MemoryArena* temporary_storage,
                u32 indentation_level);
char* to_string(Glyph* font_atlas,
                MemoryArena* temporary_storage,
                u32 indentation_level,
                u32* bytes_written);

void print_font_atlas_catalog(FontAtlasCatalog* font_atlas_catalog);
void save_font_atlas_catalog(FontAtlasCatalog* font_atlas_catalog);
#endif
