#ifndef TEXTURE_ARRAY_H
#define TEXTURE_ARRAY_H

#include "T1_platform_layer.h"
#include "T1_logger.h"
#include "T1_memorystore.h"
#include "T1_decodedimage.h"
#include "debigulator/src/decode_png.h"
#include "debigulator/src/decode_bmp.h"
#include "T1_cpu_gpu_shared_types.h"


#define MAX_ASSET_FILENAME_SIZE 30
#define MAX_ASSET_FILES 1500

#define TEXTUREARRAY_FILENAME_SIZE 128
typedef struct TextureArrayImage {
    DecodedImage image;
    char filename[TEXTUREARRAY_FILENAME_SIZE];
    bool32_t request_update;
    bool32_t prioritize_asset_load;
} TextureArrayImage;

typedef struct TextureArray {
    TextureArrayImage images[MAX_FILES_IN_SINGLE_TEXARRAY];
    uint32_t images_size;
    uint32_t single_img_width;
    uint32_t single_img_height;
    bool32_t gpu_initted;
    bool32_t request_init;
    bool32_t bc1_compressed;
} TextureArray;

extern TextureArray * texture_arrays;
extern uint32_t texture_arrays_size;


void T1_texture_array_init(void);

int32_t T1_texture_array_preinit_new_with_known_dimensions(
    const uint32_t single_img_width,
    const uint32_t single_img_height,
    const uint32_t image_count,
    const bool32_t use_bc1_compression);

void T1_texture_array_push_all_preregistered(void);

void T1_texture_array_push_dds_image_to_preinitted(
    const int32_t to_texturearray_i,
    const int32_t to_texture_i,
    const char * filename);

void T1_texture_array_gpu_try_push(void);

void T1_texture_files_register_new_by_splitting_file(
    const char * filename,
    const uint32_t rows,
    const uint32_t columns);

void T1_texture_array_preregister_null_image(
    const char * filename,
    const uint32_t height,
    const uint32_t width);
void T1_texture_array_preregister_null_png_from_disk(
    const char * filename);

void T1_texture_array_register_new_by_splitting_image(
    DecodedImage * new_image,
    const char * filename_prefix,
    const uint32_t rows,
    const uint32_t columns);

void T1_texture_array_register_high_priority_if_unloaded(
    const int32_t texture_array_i,
    const int32_t texture_i);

void T1_texture_array_get_filename_location(
    const char * for_filename,
    int32_t * texture_array_i_recipient,
    int32_t * texture_i_recipient);

void T1_texture_array_decode_null_png_at(
    uint8_t * freeable_rgba_values,
    const uint32_t rgba_values_size,
    const int32_t texture_array_i,
    const int32_t texture_i);

void T1_texture_array_load_font_images(void);

void T1_texture_array_decode_all_null_images(void);

void T1_texture_array_flag_all_to_request_gpu_init(void);

void T1_texture_array_debug_dump_texturearray_to_writables(
    const int32_t texture_array_i,
    uint32_t * success);

#endif // TEXTURE_ARRAY_H
