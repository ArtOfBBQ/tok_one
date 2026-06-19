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
    u8  good;
} T1Img;

#ifdef __cplusplus
extern "C" {
#endif

u64
T1_img_get_sum_rgba(const T1Img * input);

u32
T1_img_get_avg_rgba(const T1Img * input);

void
T1_img_overwrite_subregion(
    T1Img * whole_image,
    const T1Img * new_image,
    const u32 column_count,
    const u32 row_count,
    const u32 at_column,
    const u32 at_row);

#ifdef __cplusplus
}
#endif

#endif // T1_IMG_H
