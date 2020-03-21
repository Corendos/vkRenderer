#include "hash.hpp"

uint64_t hash(const char* str) {
    uint64_t hash = 0;
    int c;

    while (c = *str++) {
	hash = c + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}
