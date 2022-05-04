#ifndef TEXTURE_ARRAY_H
#define TEXTURE_ARRAY_H

#include "platform_layer.h"
#include "debigulator/src/decodedimage.h"
#include "debigulator/src/decode_png.h"
#include "vertex_types.h"

typedef struct TextureArray {
    DecodedImage * image;
    uint32_t sprite_columns;
    uint32_t sprite_rows;
    bool32_t request_update;
} TextureArray;

// #define MAX_TEXTURE_FILENAME_SIZE 30

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

/*
Next are functions to register new image(s) or imgfile(s0
into texture_arrays

Q: Why wouldn't I just modify texture_arrays directly?
A:
- The gpu needs to be informed that they need to copy the data
- The images prefer to be organized same dimensions together
*/
// returns storage location
void register_new_texturearray_from_files(
    const char ** filenames,
    const uint32_t filenames_size);

// returns storage location
void register_new_texturearray_from_images(
    const DecodedImage ** new_images,
    const uint32_t new_images_size);

#endif

