#ifndef __MACRO_HPP__
#define __MACRO_HPP__

#define free_null(x) free(x); x = 0
#define array_size(array) sizeof(array) / sizeof(array[0])
#define KB(x) x * 1024
#define MB(x) x * 1024 * 1024
#define GB(x) x * 1024 * 1024 * 1024


#endif
