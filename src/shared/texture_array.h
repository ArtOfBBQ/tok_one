#ifndef TEXTURE_ARRAY_H
#define TEXTURE_ARRAY_H

#include "../shared/platform_layer.h"
#include "../shared/decodedimage.h"
#include "../shared/decode_png.h"
#include "../shared/vertex_types.h"

typedef struct TextureArray {
    DecodedImage * image;
    uint32_t sprite_columns;
    uint32_t sprite_rows;
    bool32_t request_update;
} TextureArray;

// A buffer of texture arrays (AKA texture atlases) your
// objects can use
// Each texture atlas must have images of the exact same size
// You can set a zTriangle's texturearray_i to 2 to use
// texture_arrays[2] as its texture during texture mapping
// Set the zTriangle's texture_i to select which texture inside
// the texture atlas to use
// REMINDER: You must define TEXTUREARRAYS_SIZE in vertex_types.h
extern TextureArray texture_arrays[TEXTUREARRAYS_SIZE];
extern uint32_t texture_arrays_size;

DecodedImage * extract_image(
    TextureArray * texture_array,
    uint32_t x,
    uint32_t y);

// returns new_texture_array_i (index in texture_arrays)
int32_t register_new_texturearray_from_file(
    const char * filename);

// returns new_texture_array_i (index in texture_arrays)
int32_t register_new_texturearray(
    DecodedImage * new_image);

#endif

