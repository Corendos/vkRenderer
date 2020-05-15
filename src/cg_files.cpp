#include "cg_files.h"

inline u32 get_file_size(FILE* file) {
    u32 size = 0;
    
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    return size;
}

inline void copy_file_to(FILE* file, u8* dest, u32 file_size) {
    if (file_size == 0) {
        file_size = get_file_size(file);
    }
    
    u8* pos = dest;
    for (u32 i = 0;i < file_size;++i) {
        *pos++ = fgetc(file);
    }
}
