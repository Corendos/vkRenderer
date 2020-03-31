#include "utils.hpp"

#include <cstring>

void get_full_path_from_root(const char* filename, char* output) {
    strcpy(output, PROGRAM_ROOT);
    output[sizeof(PROGRAM_ROOT) - 1] = '/';
    strcpy(output + sizeof(PROGRAM_ROOT), filename);
}
