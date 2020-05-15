#ifndef __CG_FILES_H__
#define __CG_FILES_H__

u32 get_file_size(FILE* file);
void copy_file_to(FILE* file, u8* dest, u32 file_size = 0);

#endif //CG_FILES_H
