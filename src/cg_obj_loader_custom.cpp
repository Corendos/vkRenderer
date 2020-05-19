#include "cg_obj_loader.h"
#include "cg_vertex.h"

#include <stdio.h>

inline void obj__discard_line(FILE* file) {
    int int_c = getc(file);
    while(int_c != EOF && int_c != '\n') {
        int_c = getc(file);
    }
}

inline void obj__consume_spaces(FILE* file, char* c) {
    int int_c = getc(file);
    while (int_c == ' ' || int_c == '\t') {
        int_c = getc(file);
    }
    
    *c = (char)int_c;
}

inline bool get_face_type(FILE* file, ObjFaceType* face_type) {
    long int pos = ftell(file);
    
    char c = 0;
    obj__consume_spaces(file, &c);
    if (c == EOF) {
        // Unexpected end of file
        return false;
    }
    
    u32 slash_count = 0;
    bool successive = false;
    bool maybe_successive = false;
    
    int int_c = getc(file);
    while(int_c != ' ' && int_c != EOF) {
        c = (char)int_c;
        if (c == '/') {
            if (maybe_successive) {
                // If the last character was a '/', they are successive
                successive = true;
            }
            maybe_successive = true;
            slash_count++;
        } else {
            // If the last character was a slash, they are not successive
            maybe_successive = false;
        }
        
        // Next character
        int_c = getc(file);
    }
    
    
    fseek(file, pos, SEEK_SET);
    // If we reached the end of the file, this means the file is malformed.
    // Indeed, there is only one set of indices for a face.
    if (int_c == EOF) return false;
    
    // Deduce the face type from the collected infos
    if (slash_count == 0) {
        *face_type = OFT_Vertex;
    } else if (slash_count == 1) {
        *face_type = OFT_Texture;
    } else if (slash_count == 2) {
        if (successive) *face_type = OFT_Normal;
        else *face_type = OFT_TextureNormal;
    } else {
        // We have an unexpected number of '/'
        return false;
    }
    
    return true;
}

inline bool get_obj_file_info(FILE* file, ObjParse* obj_parse) {
    int int_c = getc(file);
    char c = 0;
    while (int_c != EOF) {
        c = (char)int_c;
        
        switch(c) {
            case 'v':
            int_c = getc(file);
            if (int_c != EOF) {
                c = (char)int_c;
                if (c == 'n') {
                    obj_parse->normal_count++;
                } else if (c == 't') {
                    obj_parse->uv_count++;
                } else if (c == ' ') {
                    obj_parse->vertex_count++;
                }
                obj__discard_line(file);
            } else {
                // Unexpected end of file
                return false;
            }
            break;
            case 'f':
            if (obj_parse->face_type == OFT_Undefined) {
                bool success = get_face_type(file, &obj_parse->face_type);
                if (!success) return false;
            }
            obj_parse->face_count++;
            obj__discard_line(file);
            break;
            default:
            obj__discard_line(file);
            break;
        }
        
        int_c = getc(file);
    }
    
    fseek(file, 0, SEEK_SET);
    
    return true;
}

inline bool load_obj_file(ConstString* filename, Vertex** vertex_buffer, u32* vertex_buffer_size, MemoryArena* storage) {
    FILE* file = fopen(filename->str, "r");
    
    if (file == 0) {
        fclose(file);
        return false;
    }
    
    ObjParse obj_parse = {};
    
    bool pre_parse = get_obj_file_info(file, &obj_parse);
    if (!pre_parse) return false;
    
    // Allocate the space fot the vertex buffer
    *vertex_buffer_size = obj_parse.face_count * 3;
    Vertex* vertices = (Vertex*)zero_allocate(storage, *vertex_buffer_size * sizeof(Vertex));
    if (vertices == 0) return false;
    
    // Allocate the temporary slots for the vertices/uv/normals
    TemporaryMemory temporary_memory = make_temporary_memory(storage);
    
    Vec3f* position = 0;
    Vec2f* uv = 0;
    Vec3f* normal = 0;
    
    if (obj_parse.vertex_count != 0) {
        position = (Vec3f*)zero_allocate(&temporary_memory, obj_parse.vertex_count * sizeof(Vec3f));
        if (position == 0) {
            destroy_temporary_memory(&temporary_memory);
            return false;
        }
    }
    
    if (obj_parse.uv_count != 0) {
        uv = (Vec2f*)zero_allocate(&temporary_memory, obj_parse.uv_count * sizeof(Vec2f));
        if (uv == 0) {
            destroy_temporary_memory(&temporary_memory);
            return false;
        }
    }
    
    if (obj_parse.normal_count != 0) {
        normal = (Vec3f*)zero_allocate(&temporary_memory, obj_parse.normal_count * sizeof(Vec3f));
        if (normal == 0) {
            destroy_temporary_memory(&temporary_memory);
            return false;
        }
    }
    
    Vec3f* current_position = position;
    Vec2f* current_uv = uv;
    Vec3f* current_normal = normal;
    
    Vertex* current_vertex = vertices;
    
    int int_c = getc(file);
    char c = 0;
    while (int_c != EOF) {
        c = (char)int_c;
        
        u32 v_index[3] = {0};
        u32 t_index[3] = {0};
        u32 n_index[3] = {0};
        
        switch(c) {
            case 'v':
            int_c = getc(file);
            if (int_c != EOF) {
                c = (char)int_c;
                if (c == 'n') {
                    // Not implemented yet
                } else if (c == 't') {
                    fscanf(file, "%f %f", &current_uv->x, &current_uv->y);
                    current_uv++;
                } else if (c == ' ') {
                    fscanf(file, "%f %f %f", &current_position->x, &current_position->y, &current_position->z);
                    current_position++;
                }
                obj__discard_line(file);
            }
            break;
            
            case 'f':
            switch (obj_parse.face_type) {
                case OFT_Vertex:
                fscanf(file, "%u %u %u", v_index, v_index + 1, v_index + 2);
                *current_vertex++ = make_vertex(position[v_index[0] - 1]);
                *current_vertex++ = make_vertex(position[v_index[1] - 1]);
                *current_vertex++ = make_vertex(position[v_index[2] - 1]);
                break;
                case OFT_Texture:
                fscanf(file, "%u/%u %u/%u %u/%u",
                       v_index, t_index,
                       v_index + 1, t_index + 1,
                       v_index + 2, t_index + 2);
                *current_vertex++ = make_vertex(position[v_index[0] - 1], uv[t_index[0] - 1]);
                *current_vertex++ = make_vertex(position[v_index[1] - 1], uv[t_index[1] - 1]);
                *current_vertex++ = make_vertex(position[v_index[2] - 1], uv[t_index[2] - 1]);
                break;
                case OFT_Normal:
                fscanf(file, "%u//%u %u//%u %u//%u",
                       v_index, n_index,
                       v_index + 1, n_index + 1,
                       v_index + 2, n_index + 1);
                *current_vertex++ = make_vertex(position[v_index[0] - 1], normal[n_index[0] - 1]);
                *current_vertex++ = make_vertex(position[v_index[1] - 1], normal[n_index[1] - 1]);
                *current_vertex++ = make_vertex(position[v_index[2] - 1], normal[n_index[2] - 1]);
                break;
                case OFT_TextureNormal:
                fscanf(file, "%u/%u/%u %u/%u/%u %u/%u/%u",
                       v_index, t_index, n_index,
                       v_index + 1, t_index + 1, n_index + 1,
                       v_index + 2, t_index + 2, n_index + 1);
                *current_vertex++ = make_vertex(position[v_index[0] - 1], uv[t_index[0] - 1], normal[n_index[0] - 1]);
                *current_vertex++ = make_vertex(position[v_index[1] - 1], uv[t_index[1] - 1], normal[n_index[1] - 1]);
                *current_vertex++ = make_vertex(position[v_index[2] - 1], uv[t_index[2] - 1], normal[n_index[2] - 1]);
                break;
                default:
                break;
            }
            obj__discard_line(file);
            break;
            
            default:
            obj__discard_line(file);
            break;
        }
        
        int_c = getc(file);
    }
    
    destroy_temporary_memory(&temporary_memory);
    fclose(file);
    
    *vertex_buffer = vertices;
    
    return true;
}