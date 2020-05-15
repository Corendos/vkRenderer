#ifndef __MACRO_H__
#define __MACRO_H__

#define free_null(x) free(x); x = 0
#define array_size(array) (u64)(sizeof((array)) / sizeof((array)[0]))
#define KB(x) x * 1024
#define MB(x) x * 1024 * 1024
#define GB(x) x * 1024 * 1024 * 1024

#define println(format, ...) printf(format "\n", ##__VA_ARGS__)

#define print printf


#endif
