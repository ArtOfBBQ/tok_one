#ifndef TEXTURE_ARRAY_H
#define TEXTURE_ARRAY_H

#include "debigulator/src/decode_png.h"
#include "debigulator/src/decode_bmp.h"

#include "T1_tex.h"
#include "T1_platform_layer.h"
#include "T1_logger.h"
#include "T1_mem.h"
#include "T1_decodedimage.h"
#include "T1_cpu_gpu_shared_types.h"

#define MAX_ASSET_FILENAME_SIZE 30
#define MAX_ASSET_FILES 1500

#define TEXTUREARRAY_FILENAME_SIZE 128
typedef struct {
    T1DecodedImage image;
    char name[TEXTUREARRAY_FILENAME_SIZE];
    bool8_t deleted;
    bool8_t request_update;
    bool8_t prioritize_asset_load;
} T1TextureArrayImage;

typedef struct {
    T1TextureArrayImage images[MAX_FILES_IN_SINGLE_TEXARRAY];
    uint64_t started_decoding;
    uint64_t ended_decoding;
    uint32_t images_size;
    uint32_t single_img_width;
    uint32_t single_img_height;
    uint32_t gpu_capacity;
    bool8_t request_init;
    bool8_t is_render_target;
    bool8_t bc1_compressed;
    bool8_t deleted;
} T1TextureArray;

extern T1TextureArray * T1_texture_arrays;
extern uint32_t T1_texture_arrays_size;


void T1_texture_array_init(void);

void T1_texture_array_push_all_predecoded(void);

void T1_texture_array_preregister_null_image(
    const char * filename,
    const uint32_t width,
    const uint32_t height,
    const uint32_t is_render_target,
    const uint32_t use_bc1_compression);
void T1_texture_array_postregister_null_image(
    const char * filename,
    const uint32_t width,
    const uint32_t height,
    const uint32_t is_render_target,
    const uint32_t use_bc1_compression);

int32_t T1_texture_array_create_new_render_view(
    const uint32_t width,
    const uint32_t height);

void T1_texture_array_delete_array(
    const int32_t array_i);

void T1_texture_array_delete_slice(
    const int32_t array_i,
    const int32_t slice_i);

void T1_texture_array_register_new_by_splitting_image(
    T1DecodedImage * new_image,
    const char * filename_prefix,
    const uint32_t rows,
    const uint32_t columns);

T1Tex T1_texture_array_get_filename_location(
    const char * for_filename);

void T1_texture_array_debug_dump_texturearray_to_writables(
    const int32_t texture_array_i,
    uint32_t * success);

#endif // TEXTURE_ARRAY_H
