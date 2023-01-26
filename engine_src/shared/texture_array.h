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

void init_texture_arrays(void);
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
    const int32_t at_texture_i,
    uint8_t * dpng_working_memory,
    uint64_t dpng_working_memory_size);

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

void decode_null_image_with_memory(
    const int32_t texture_array_i,
    const int32_t texture_i,
    uint8_t * dpng_working_memory,
    const uint64_t dpng_working_memory_size);

void decode_all_null_images_with_memory(
    uint8_t * dpng_working_memory,
    const uint64_t dpng_working_memory_size);

void flag_all_texture_arrays_to_request_gpu_init(void);

bool32_t texture_has_alpha_channel(
    const int32_t texturearray_i,
    const int32_t texture_i);

#ifdef __cplusplus
}
#endif

#endif // TEXTURE_ARRAY_H
