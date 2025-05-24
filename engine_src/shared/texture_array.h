#ifndef TEXTURE_ARRAY_H
#define TEXTURE_ARRAY_H

#include "memorystore.h"
#include "platform_layer.h"
#include "logger.h"
#include "decodedimage.h"
#include "debigulator/src/decode_png.h"
#include "debigulator/src/decode_bmp.h"
#include "cpu_gpu_shared_types.h"

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

void texture_array_gpu_try_push(void);

void texture_array_register_new_by_splitting_file(
    const char * filename,
    const uint32_t rows,
    const uint32_t columns);

void texture_array_preregister_null_image(
    const char * filename,
    const uint32_t height,
    const uint32_t width);
void texture_array_preregister_null_png_from_disk(
    const char * filename);

void texture_array_register_high_priority_if_unloaded(
    const int32_t texture_array_i,
    const int32_t texture_i);

void texture_array_get_filename_location(
    const char * for_filename,
    int32_t * texture_array_i_recipient,
    int32_t * texture_i_recipient);

void texture_array_decode_null_image_at(
    const int32_t texture_array_i,
    const int32_t texture_i);

void texture_array_load_font_images(void);

void texture_array_decode_all_null_images(void);

void texture_array_flag_all_to_request_gpu_init(void);

#endif // TEXTURE_ARRAY_H
