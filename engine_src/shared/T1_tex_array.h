#ifndef T1_TEX_ARRAY_H
#define T1_TEX_ARRAY_H

#include "debigulator/src/decode_png.h"
#include "debigulator/src/decode_bmp.h"

#include "T1_tex.h"
#include "T1_platform_layer.h"
#include "T1_log.h"
#include "T1_mem.h"
#include "T1_img.h"
#include "T1_cpu_gpu_shared_types.h"

#define T1_TEX_NAME_CAP 128
typedef struct {
    T1Img image;
    char name[T1_TEX_NAME_CAP];
    bool8_t deleted;
    bool8_t request_update;
    bool8_t prioritize_asset_load;
} T1TexArrayImg;

typedef struct {
    T1TexArrayImg images[T1_TEX_SLICES_CAP];
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
} T1TexArray;

extern T1TexArray * T1_tex_arrays;
extern uint32_t T1_tex_arrays_size;

void
T1_tex_array_init(void);

void
T1_tex_array_push_all(void);

T1Tex T1_tex_array_reg_img(
    const char * filename,
    const uint32_t width,
    const uint32_t height,
    const uint32_t is_render_target,
    const uint32_t use_bc1_compression);

int32_t T1_tex_array_create_new_render_view(
    const uint32_t width,
    const uint32_t height);

void T1_tex_array_delete_array(
    const int32_t array_i);

void T1_tex_array_delete_slice(
    const int32_t array_i,
    const int32_t slice_i);

void T1_tex_array_update_rgba(
    const int32_t array_i,
    const int32_t slice_i,
    const uint8_t * rgba,
    const uint32_t rgba_size);

void T1_tex_array_reg_new_by_splitting_img(
    T1Img * new_image,
    const char * filename_prefix,
    const uint32_t rows,
    const uint32_t columns);

T1Tex T1_tex_array_create_new_in_array(
    const int32_t array_i);

T1Tex T1_tex_array_get_filename_loc(
    const char * for_filename);

void T1_tex_array_debug_dump_to_writables(
    const int32_t texture_array_i,
    uint32_t * success);

#endif // T1_TEX_ARRAY_H
