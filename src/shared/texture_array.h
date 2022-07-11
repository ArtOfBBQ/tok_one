#ifndef TEXTURE_ARRAY_H
#define TEXTURE_ARRAY_H

#include "memorystore.h"
#include "platform_layer.h"
#include "logger.h"
#include "decodedimage.h"
#include "debigulator/src/decode_png.h"
#include "vertex_types.h"

#define MAX_ASSET_FILENAME_SIZE 30
#define MAX_FILES_IN_SINGLE_TEXARRAY 200
#define MAX_ASSET_FILES 1500

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

DecodedImage * malloc_img_from_filename(
    const char * filename);

void debug_dump_texturearrays_to_disk();

void update_texturearray_from_0terminated_files(
    const int32_t texturearray_i,
    const char filenames
        [MAX_FILES_IN_SINGLE_TEXARRAY]
        [MAX_ASSET_FILENAME_SIZE]);

void update_texturearray_from_0terminated_files_with_memory(
    const int32_t texturearray_i,
    const char filenames
        [MAX_FILES_IN_SINGLE_TEXARRAY]
        [MAX_ASSET_FILENAME_SIZE],
    const uint8_t * inflate_working_memory,
    const uint64_t inflate_working_memory_size);

/*
Next are functions to register new image(s) or imgfile(s0
into texture_arrays

Q: Why wouldn't I just modify texture_arrays directly?
A:
- The gpu needs to be informed that they need to copy the data
- The images prefer to be organized same dimensions together
*/
void register_new_texturearray_from_files(
    const char ** filenames,
    const uint32_t filenames_size);

void register_new_texturearray_from_images(
    DecodedImage ** new_images,
    const uint32_t new_images_size);

void register_new_texturearray(
    DecodedImage * new_image);

#endif

