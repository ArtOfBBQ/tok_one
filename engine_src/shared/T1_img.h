#ifndef T1_IMG_H
#define T1_IMG_H

#include "T1_stdint.h"

typedef struct {
    u32 rgba_values_size;
    u32 width;
    u32 height;
    u32 pixel_count; // rgba_values_size / 4
    b8  never_delete_me; // T1_tex_array_delete_slice() leaves it
} T1Img;

#endif // T1_IMG_H
