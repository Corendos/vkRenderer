#ifndef __CG_OBJ_LOADER__
#define __CG_OBJ_LOADER__

#ifdef OBJ_CUSTOM
#include "cg_obj_loader_custom.h"
#else
#include "cg_obj_loader_assimp.h"
#endif

#include "cg_string.h"
#include "cg_memory_arena.h"
#include "cg_vertex.h"

bool load_obj_file(ConstString* filename, Vertex** vertex_buffer, u32* vertex_buffer_size, MemoryArena* storage);

#endif //CG_OBJ_LOADER