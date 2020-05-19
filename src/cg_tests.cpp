#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "cg_macros.h"
#include "cg_string.h"
#include "cg_timer.h"
#include "cg_math.h"

#include "cg_string.cpp"
#include "cg_timer.cpp"
#include "cg_math.cpp"

#define STRING_SIZE 20

Mat4f random_matrix() {
    Mat4f m = {};
    for (u32 i = 0;i < 16;++i) {
        m.v[i] = randf() * 10.0f;
    }
    
    return m;
}

void print_matrix(Mat4f m) {
    println("Mat4f {\n"
            "    %+9.6f %+9.6f %+9.6f %+9.6f\n"
            "    %+9.6f %+9.6f %+9.6f %+9.6f\n"
            "    %+9.6f %+9.6f %+9.6f %+9.6f\n"
            "    %+9.6f %+9.6f %+9.6f %+9.6f\n"
            "}",
            m.m00, m.m01, m.m02, m.m03,
            m.m10, m.m11, m.m12, m.m13,
            m.m20, m.m21, m.m22, m.m23,
            m.m30, m.m31, m.m32, m.m33);
}

int main() {
    srand(get_time_ns());
    
    u64 cumulated_time = 0;
    u32 repetition = 100000;
    
    for (u32 i = 0;i < repetition;++i) {
        Mat4f m = random_matrix();
        
        u64 start = get_time_ns();
        asm volatile("" : : : "memory");
        Mat4f im = inverse(&m);
        asm volatile("" : : "g"(im) : );
        u64 end = get_time_ns();
        print_matrix(im);
        cumulated_time += (end - start);
    }
    
    println("Computed %d matrix inversion in %lu ns", repetition, cumulated_time);
    return 0;
}