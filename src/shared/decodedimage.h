/*
This file contains an image structure and some methods
to play with it.
*/

#ifndef DECODED_IMAGE_H
#define DECODED_IMAGE_H

#define bool32_t uint32_t

#include "inttypes.h"
#include "stdlib.h"

#ifndef DECODED_IMAGE_SILENCE
#include "stdio.h"
#endif

#ifndef DECODED_IMAGE_IGNORE_ASSERTS
#include "assert.h"
#endif

typedef struct DecodedImage {
    uint8_t * rgba_values;
    uint32_t rgba_values_size;
    uint32_t width;
    uint32_t height;
    uint32_t pixel_count; // rgba_values_size / 4
    bool32_t good;
} DecodedImage;

#endif

