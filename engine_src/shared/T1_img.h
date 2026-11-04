#ifndef T1_IMG_H
#define T1_IMG_H

#include "T1_stdint.h"

typedef struct {
    u8 * rgba_values_freeable;
    u8 * rgba_values_page_aligned;
    u32 rgba_values_size;
    u32 width;
    u32 height;
    u32 pixel_count; // rgba_values_size / 4
    b8  never_delete_me; // rgba_values_freeable must be NULL
} T1Img;

#endif // T1_IMG_H
