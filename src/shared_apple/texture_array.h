#ifndef SPLIT_IMAGE_H
#define SPLIT_IMAGE_H

#include "../shared/software_renderer.h"
#include "../shared/decodedimage.h"

DecodedImage * extract_image(
    TextureArray * texture_array,
    uint32_t x,
    uint32_t y);

#endif

