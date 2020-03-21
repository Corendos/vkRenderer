#ifndef __MACRO_HPP__
#define __MACRO_HPP__

#define free_null(x) free(x); x = 0
#define array_size(array) sizeof(array) / sizeof(array[0])
#define kilo(x) x * 1024
#define mega(x) x * 1024 * 1024
#define giga(x) x * 1024 * 1024 * 1024


#endif
