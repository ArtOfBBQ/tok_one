#ifndef T1_IMG_H
#define T1_IMG_H

#define bool32_t uint32_t

#include "inttypes.h"

#define T1_IMG_SILENCE
#ifndef T1_IMG_SILENCE
#include "stdio.h"
#endif

#ifndef T1_IMG_IGNORE_ASSERTS
#include "assert.h"
#endif

typedef struct {
    uint8_t * rgba_values_freeable;
    uint8_t * rgba_values_page_aligned;
    uint32_t rgba_values_size;
    uint32_t width;
    uint32_t height;
    uint32_t pixel_count; // rgba_values_size / 4
    bool32_t good;
} T1Img;

#ifdef __cplusplus
extern "C" {
#endif

uint64_t
T1_img_get_sum_rgba(const T1Img * input);

uint32_t
T1_img_get_avg_rgba(const T1Img * input);

void
T1_img_overwrite_subregion(
    T1Img * whole_image,
    const T1Img * new_image,
    const uint32_t column_count,
    const uint32_t row_count,
    const uint32_t at_column,
    const uint32_t at_row);

#ifdef __cplusplus
}
#endif

#endif // T1_IMG_H
