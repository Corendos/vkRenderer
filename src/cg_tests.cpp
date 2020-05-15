#include <stdio.h>
#include <stdlib.h>

#include "cg_string.h"
#include "cg_timer.h"

#include "cg_string.cpp"
#include "cg_timer.cpp"

#define STRING_SIZE 20

int main() {
    char *data = (char*)malloc(STRING_SIZE + 1);
    ConstString string = make_const_string(data, STRING_SIZE);
    
    for (u32 i = 0;i < 1000000;++i) {
        u64 n = string_format(string, "test witha %d", 64123123);
    }
    println("%lu", string.size);
    
    println("string = '%s'", string.str);
    
    free(data);
    
    return 0;
}