#ifndef T1_TEX_ARRAY_H
#define T1_TEX_ARRAY_H

#include "T1_std.h"
#include "T1_tex.h"
#include "T1_img.h"

typedef struct {
    T1Img image;
    b8 deleted;
} T1TexArrayImg;

typedef struct {
    T1TexArrayImg images[T1_TEX_SLICES_CAP];
    u32 images_size;
    u32 single_img_width;
    u32 single_img_height;
    u32 gpu_capacity;
    b8 deleted;
} T1TexArray;

extern T1TexArray * T1_tex_arrays;
extern u32 T1_tex_arrays_size;

void T1_tex_array_init(void);


// a slice in an array of this size (made if needed), its pixels are unset
T1Tex T1_tex_array_new_slice(
    u32 width,
    u32 height,
    b8 never_delete,
    char ** const fatal_error);

u32 T1_tex_array_get_img_height(s32 array_i);
u32 T1_tex_array_get_img_width(s32 array_i);

void T1_tex_array_delete_array(s32 array_i);

void T1_tex_array_delete_slice(s32 array_i, s32 slice_i);

const u8 * T1_tex_array_get_const_rgba(
    s32 array_i,
    s32 slice_i);

void T1_tex_array_update_rgba(
    s32 array_i,
    s32 slice_i,
    const u8 * rgba,
    u32 rgba_size);

void T1_tex_array_reg_new_by_splitting_img(
    T1Img * new_image,
    u32 rows,
    u32 columns,
    b8 free_rgba,
    char ** const fatal_error);


/*
Public functions (exposed via T1.h)
*/
T1Tex T1_tex_new(
    const u8 * rgba,
    u32 width,
    u32 height,
    char ** const fatal_error);
void T1_tex_update(
    T1Tex tex,
    const u8 * rgba);
void T1_tex_delete(T1Tex tex);
u32 T1_tex_get_width(T1Tex tex);
u32 T1_tex_get_height(T1Tex tex);


void T1_tex_array_debug_dump_to_writables(
    s32 texture_array_i,
    u32 * success);

#endif // T1_TEX_ARRAY_H
