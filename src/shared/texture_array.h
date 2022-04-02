#ifndef TEXTURE_ARRAY_H
#define TEXTURE_ARRAY_H

#include "../shared/decodedimage.h"

typedef struct TextureArray {
    DecodedImage * image;
    uint32_t sprite_columns;
    uint32_t sprite_rows;
    bool32_t request_update;
} TextureArray;

DecodedImage * extract_image(
    TextureArray * texture_array,
    uint32_t x,
    uint32_t y);

#endif

