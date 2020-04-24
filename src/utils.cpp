#include "utils.hpp"

#include <string.h>

void get_full_path_from_root(const char* filename, char* output) {
    sprintf(output, "%s/%s", PROGRAM_ROOT, filename);
}
