#include "cg_utils.h"

#include <string.h>

inline void get_full_path_from_root(const char* filename, char* output) {
    sprintf(output, "%s/%s", PROGRAM_ROOT, filename);
}
