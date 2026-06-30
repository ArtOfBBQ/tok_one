#ifndef T1_TEX_ARRAY_H
#define T1_TEX_ARRAY_H

#include "T1_std.h"
#include "T1_tex.h"
#include "T1_img.h"


#define T1_TEX_NAME_CAP 128
typedef struct {
    T1Img image;
    char name[T1_TEX_NAME_CAP];
    b8 deleted;
    b8 request_update;
    b8 prioritize_asset_load;
} T1TexArrayImg;

typedef struct {
    T1TexArrayImg images[T1_TEX_SLICES_CAP];
    u64 started_decoding;
    u64 ended_decoding;
    u32 images_size;
    u32 single_img_width;
    u32 single_img_height;
    u32 gpu_capacity;
    b8 request_init;
    b8 is_render_target;
    b8 bc1_compressed;
    b8 deleted;
} T1TexArray;

extern T1TexArray * T1_tex_arrays;
extern u32 T1_tex_arrays_size;

void
T1_tex_array_init(void);

void
T1_tex_array_push_all(void);

T1Tex T1_tex_array_reg_img(
    const c8 * filename,
    u32 width,
    u32 height,
    b8 is_render_target,
    b8 use_bc1_compression);

s32 T1_tex_array_create_new_render_view(
    const u32 width,
    const u32 height);

void T1_tex_array_delete_array(
    const s32 array_i);

void T1_tex_array_delete_slice(
    const s32 array_i,
    const s32 slice_i);

void T1_tex_array_update_rgba(
    s32 array_i,
    s32 slice_i,
    const u8 * rgba,
    u32 rgba_size);

void T1_tex_array_reg_new_by_splitting_img(
    T1Img * new_image,
    const char * filename_prefix,
    const u32 rows,
    const u32 columns);

T1Tex T1_tex_array_create_new_in_array(
    const s32 array_i);

T1Tex T1_tex_array_get_filename_loc(
    const char * for_filename);

void T1_tex_array_debug_dump_to_writables(
    const s32 texture_array_i,
    u32 * success);

#endif // T1_TEX_ARRAY_H
