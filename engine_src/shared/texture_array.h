#ifndef TEXTURE_ARRAY_H
#define TEXTURE_ARRAY_H

#include "memorystore.h"
#include "platform_layer.h"
#include "logger.h"
#include "decodedimage.h"
#include "debigulator/src/decode_png.h"
#include "debigulator/src/decode_bmp.h"
#include "cpu_gpu_shared_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void texture_array_init(void);

int32_t texture_array_preinit_new_with_known_dimensions(
    const uint32_t single_img_width,
    const uint32_t single_img_height,
    const uint32_t image_count,
    const bool32_t use_bc1_compression);

void texture_array_push_dds_image_to_preinitted(
    const int32_t to_texturearray_i,
    const int32_t to_texture_i,
    const char * filename);

void init_or_push_one_gpu_texture_array_if_needed(void);

void register_new_texturearray_from_files(
    const char ** filenames,
    const uint32_t filenames_size);

void register_to_texturearray_by_splitting_file(
    const char * filename,
    const int32_t texture_array_i,
    const uint32_t rows,
    const uint32_t columns);

void register_new_texturearray_by_splitting_file(
    const char * filename,
    const uint32_t rows,
    const uint32_t columns);

void update_texture_slice_from_file_with_memory(
    const char * filename,
    const int32_t at_texture_array_i,
    const int32_t at_texture_i);

void update_texture_slice(
    DecodedImage * new_image,
    const int32_t at_texture_array_i,
    const int32_t at_texture_i);

void preregister_null_image(
    char * filename,
    const uint32_t height,
    const uint32_t width);
void preregister_file_as_null_image(
    char * filename);

void register_high_priority_if_unloaded(
    const int32_t texture_array_i,
    const int32_t texture_i);

void get_texture_location(
    char * for_filename,
    int32_t * texture_array_i_recipient,
    int32_t * texture_i_recipient);

void decode_null_image_at(
    const int32_t texture_array_i,
    const int32_t texture_i);

void load_font_images(void);
void decode_all_null_images(void);

void flag_all_texture_arrays_to_request_gpu_init(void);

bool32_t texture_has_alpha_channel(
    const int32_t texturearray_i,
    const int32_t texture_i);

#ifdef __cplusplus
}
#endif

#endif // TEXTURE_ARRAY_H
