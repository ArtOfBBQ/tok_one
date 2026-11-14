#ifndef T1_TEX_ARRAY_H
#define T1_TEX_ARRAY_H

#include "T1_std.h"
#include "T1_tex.h"
#include "T1_img.h"
#include "T1_macro_settings.h"

/*
An array's format: what its updates take (T1_tex_new_rgba(), _bc1(),
_bc3()) and, on a GPU that samples BC, how it keeps them. A GPU without BC
keeps a BC array as rgba8 (release builds only, see T1_tex_new_bc1()) and
decodes its updates (T1_bc.h). Arrays of one size but another format are
arrays of their own. A BC array is only sampled (no pass draws or writes
into it, no reads) and has one mip level
*/
typedef u8 T1TexFormat;
enum {
    T1_TEX_FORMAT_RGBA8 = 0,
    T1_TEX_FORMAT_BC1,
    T1_TEX_FORMAT_BC3,
};

// the bytes of a row of pixels (a compressed one: of a row of 4x4 blocks)
u32 T1_tex_format_row_bytes(T1TexFormat format, u32 width);
// the bytes of a whole image
u32 T1_tex_format_bytes(T1TexFormat format, u32 width, u32 height);

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
    T1TexFormat takes; // what its updates take
    T1TexFormat format; // how the GPU keeps it (see T1TexFormat)
    b8 deleted;
} T1TexArray;

extern T1TexArray * T1_tex_arrays;
extern u32 T1_tex_arrays_size;

void T1_tex_array_init(void);


/*
A slice in an array of this size that takes these updates, kept in this
format (made if needed), its pixels unset
*/
T1Tex T1_tex_array_new_slice(
    u32 width,
    u32 height,
    T1TexFormat takes,
    T1TexFormat format,
    b8 never_delete,
    char ** const fatal_error);

void T1_tex_array_delete_array(s32 array_i);

void T1_tex_array_delete_slice(s32 array_i, s32 slice_i);

// whether tex takes BC blocks (only sampled, on every GPU)
b8 T1_tex_is_compressed(T1Tex tex);



/*
Public functions (exposed via T1.h)
*/
T1Tex T1_tex_new_rgba(u32 width, u32 height, char ** const fatal_error);
T1Tex T1_tex_new_bc1(u32 width, u32 height, char ** const fatal_error);
T1Tex T1_tex_new_bc3(u32 width, u32 height, char ** const fatal_error);
void T1_tex_update_from_rgba(
    T1Tex tex,
    u32 x,
    u32 y,
    u32 width,
    u32 height,
    const u8 * rgba);
void T1_tex_update_from_bc1(
    T1Tex tex,
    u32 x,
    u32 y,
    u32 width,
    u32 height,
    const u8 * bc1);
void T1_tex_update_from_bc3(
    T1Tex tex,
    u32 x,
    u32 y,
    u32 width,
    u32 height,
    const u8 * bc3);
void T1_tex_delete(T1Tex tex);
u32 T1_tex_get_width(T1Tex tex);
u32 T1_tex_get_height(T1Tex tex);

#endif // T1_TEX_ARRAY_H
