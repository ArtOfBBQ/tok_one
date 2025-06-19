#ifndef TEXTURE_ARRAY_H
#define TEXTURE_ARRAY_H

#include "debigulator/src/decode_png.h"
#include "debigulator/src/decode_bmp.h"

#include "T1_platform_layer.h"
#include "T1_logger.h"
#include "T1_memorystore.h"
#include "T1_decodedimage.h"
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
    uint64_t started_decoding;
    uint64_t ended_decoding;
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

void T1_texture_array_push_all_predecoded(void);

void T1_texture_files_register_new_by_splitting_file(
    const char * filename,
    const uint32_t rows,
    const uint32_t columns);

void T1_texture_array_preregister_null_image(
    const char * filename,
    const uint32_t height,
    const uint32_t width,
    const uint32_t use_bc1_compression);

void T1_texture_array_register_new_by_splitting_image(
    DecodedImage * new_image,
    const char * filename_prefix,
    const uint32_t rows,
    const uint32_t columns);

void T1_texture_array_get_filename_location(
    const char * for_filename,
    int32_t * texture_array_i_recipient,
    int32_t * texture_i_recipient);

void T1_texture_array_load_font_images(void);

void T1_texture_array_debug_dump_texturearray_to_writables(
    const int32_t texture_array_i,
    uint32_t * success);

#endif // TEXTURE_ARRAY_H
