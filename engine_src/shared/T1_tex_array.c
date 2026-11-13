#include "T1_tex_array.h"

#include "T1_assert.h"
#include "T1_mem.h"
#include "T1_os.h"
#include "T1_bc.h"
#include "T1_etc.h"
// #include "T1_cpu_gpu_shared_types.h"

static u32 tex_arrays_mutex_ids[T1_TEXARRAYS_CAP];

T1TexArray * T1_tex_arrays = NULL;
u32 T1_tex_arrays_size = 0;

void T1_tex_array_init(void) {
    
    // initialize texture arrays
    T1_tex_arrays = (T1TexArray *)
        T1_mem_malloc_unmanaged(
            sizeof(T1TexArray) *
                T1_TEXARRAYS_CAP);
    
    T1_std_memset(
        T1_tex_arrays,
        0,
        sizeof(T1TexArray) * T1_TEXARRAYS_CAP);
    
    for (u32 i = 0; i < T1_TEXARRAYS_CAP; i++)
    {
        tex_arrays_mutex_ids[i] = T1_os_init_mutex_and_return_id();
    }
}

void T1_tex_array_delete_array(
    s32 array_i)
{
    // array 0 too: e.g. an app's only texture, its window's, when the
    // window gets a new size. T1_tex_array_new_slice() reuses it
    T1_assert(array_i >= 0 && (u32)array_i < T1_tex_arrays_size);
    for (u32 slice_i = 0; slice_i < T1_tex_arrays[array_i].images_size; slice_i++) {
        T1_assert(!T1_tex_arrays[array_i].images[slice_i].image.never_delete_me);
    }
    
    T1_os_gpu_delete_texture_array(array_i);
    
    T1_std_memset(
        T1_tex_arrays + array_i,
        0,
        sizeof(T1TexArray));
    T1_tex_arrays[array_i].deleted = 1;
}

void T1_tex_array_delete_slice(
    s32 array_i,
    s32 slice_i)
{
    T1_assert(array_i >= 0);
    T1_assert(slice_i >= 0);
    
    T1_assert(
        slice_i < (s32)
            T1_tex_arrays[array_i].images_size);
    T1_assert(
        !T1_tex_arrays[array_i].images[slice_i].
            deleted);
    
    if (T1_tex_arrays[array_i].images[slice_i].image.never_delete_me) {
        return;
    }
    
    T1_assert(
        array_i < (s32)T1_tex_arrays_size);
    
    T1_std_memset(
        T1_tex_arrays[array_i].images + slice_i,
        0,
        sizeof(T1TexArrayImg));
    T1_tex_arrays[array_i].images[slice_i].deleted = 1;
    
    while (
        T1_tex_arrays[array_i].images_size > 0 &&
        T1_tex_arrays[array_i].images[
            T1_tex_arrays[array_i].images_size - 1].
                deleted)
    {
        T1_tex_arrays[array_i].images_size -= 1;
    }
    
    if (
        T1_tex_arrays[array_i].images_size == 0)
    {
        T1_tex_array_delete_array(array_i);
    }
}

u32 T1_tex_format_row_bytes(T1TexFormat format, u32 width) {
    if (format == T1_TEX_FORMAT_RGBA8) { return width * 4; }
    // (both compressed formats: 16 bytes a 4x4 block)
    return width / 4 * (format == T1_TEX_FORMAT_BC3 ?
        T1_BC3_BLOCK_BYTES : T1_ETC2_EAC_BLOCK_BYTES);
}

u32 T1_tex_format_bytes(T1TexFormat format, u32 width, u32 height) {
    if (format == T1_TEX_FORMAT_RGBA8) { return width * height * 4; }
    return T1_tex_format_row_bytes(format, width) * (height / 4);
}

b8 T1_tex_is_compressed(T1Tex tex) {
    if (tex == T1_TEX_NONE) { return 0; }
    const s16 array_i = T1_tex_to_array_i(tex);
    T1_assert(array_i >= 0 && array_i < (s16)T1_tex_arrays_size);
    return T1_tex_arrays[array_i].format != T1_TEX_FORMAT_RGBA8;
}

T1Tex T1_tex_array_new_slice(
    u32 width,
    u32 height,
    T1TexFormat format,
    b8 never_delete,
    char ** const fatal_error)
{
    T1Tex retval = T1_TEX_NONE;
    if (*fatal_error != 0) { return retval; }
    
    T1_assert(width > 0);
    T1_assert(height > 0);
    
    // *****************
    // find out if same width/height was already used
    // set new_texturearray_i and new_texture_i if so
    for (
        s16 i = 0;
        i < (s16)T1_tex_arrays_size;
        i++)
    {
        if (*fatal_error != 0) { return retval; }
        
        T1_assert(i < T1_TEXARRAYS_CAP);
        if (
            !T1_tex_arrays[i].deleted &&
            T1_tex_arrays[i].images_size > 0 &&
            T1_tex_arrays[i].single_img_width  == width &&
            T1_tex_arrays[i].single_img_height == height &&
            T1_tex_arrays[i].format == format)
        {
            // reuse a deleted slice if there is one, else append
            s16 slice_i = (s16)T1_tex_arrays[i].images_size;
            for (s16 j = 1; j < (s16)T1_tex_arrays[i].images_size; j++) {
                if (T1_tex_arrays[i].images[j].deleted) {
                    slice_i = j;
                    break;
                }
            }
            if (slice_i >= T1_TEX_SLICES_CAP) {
                // this array is full, look for (or make) another one
                continue;
            }
            T1_tex_set_array_i(&retval, i);
            T1_tex_set_slice_i(&retval, slice_i);
            if (slice_i == (s16)T1_tex_arrays[i].images_size) {
                T1_tex_arrays[i].images_size++;
            }
            T1_tex_arrays[i].images[slice_i].deleted = 0;
            
            T1Img * img = &T1_tex_arrays[i].images[slice_i].image;
            img->width = width;
            img->height = height;
            img->rgba_values_size = width * height * 4;
            img->never_delete_me = never_delete;
            
            T1_assert(slice_i >= 1);
            T1_assert(slice_i < T1_TEX_SLICES_CAP);
            break;
        }
    }
    
    // if this is a new texturearray (we never saw
    // these image dimensions before)
    if (retval == T1_TEX_NONE) {
        s16 array_i = 0;
        while (
            array_i < (s32)T1_tex_arrays_size &&
            T1_tex_arrays[array_i].images_size > 0)
        {
            array_i += 1;
        }
        
        if (array_i == (s32)T1_tex_arrays_size)
        {
            if (T1_tex_arrays_size >= T1_TEXARRAYS_CAP) {
                *fatal_error =
                    "Out of texture arrays, too many different image sizes";
                return T1_TEX_NONE;
            }
            T1_tex_arrays_size++;
        }
        
        T1_assert(T1_tex_arrays_size <= T1_TEXARRAYS_CAP);
        T1_tex_set_array_i(&retval, array_i);
        T1_tex_set_slice_i(&retval, 0);
        
        T1_tex_arrays[array_i].deleted = 0;
        T1_tex_arrays[array_i].images_size = 1;
        T1_tex_arrays[array_i].single_img_width = width;
        T1_tex_arrays[array_i].single_img_height = height;
        T1_tex_arrays[array_i].format = format;
        
        T1Img * img = &T1_tex_arrays[array_i].images[0].image;
        img->width = width;
        img->height = height;
        img->rgba_values_size = width * height * 4;
        img->never_delete_me = never_delete;
    } else {
        T1_assert(T1_tex_to_slice_i(retval) > 0);
    }
    
    T1_assert(retval != T1_TEX_NONE);
    
    return retval;
}

T1Tex T1_tex_new(
    u32 width,
    u32 height,
    b8 compress_if_possible,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return T1_TEX_NONE; }
    
    if (width < 1 || height < 1) {
        *fatal_error = "T1_tex_new() needs a width and height";
        return T1_TEX_NONE;
    }
    
    // compressed if it can be: whole 4x4 blocks, and a GPU that samples a
    // compressed format (else rgba8, as asked or not)
    const T1TexFormat format =
        compress_if_possible && width % 4 == 0 && height % 4 == 0 ?
            (T1TexFormat)T1_os_gpu_compressed_format() : T1_TEX_FORMAT_RGBA8;
    
    T1Tex tex = T1_tex_array_new_slice(
        width,
        height,
        format,
        /* b8 never_delete: */ 0,
        fatal_error);
    if (*fatal_error != 0) { return T1_TEX_NONE; }
    
    const s32 array_i = T1_tex_to_array_i(tex);
    if (format == T1_TEX_FORMAT_RGBA8) {
        // blank until a pass draws to it or it's updated
        T1_os_gpu_update_capacity_if_needed(array_i);
        return tex;
    }
    // (black: a compressed one is never drawn into, only updated)
    const u32 bytes = T1_tex_format_bytes(format, width, height);
    u8 * zeros = T1_mem_malloc_managed(bytes);
    T1_std_memset(zeros, 0, bytes);
    T1_os_gpu_write_tex_slice(
        array_i, T1_tex_to_slice_i(tex), 0, 0, width, height, zeros);
    T1_mem_free_managed(zeros);
    return tex;
}

/*
Whether (x, y, w, h) is a part of tex an update can take: inside it, in
whole 4x4 blocks except where it reaches the edge (a compressed texture
takes whole blocks, and every texture takes the same parts)
*/
static b8 T1_tex_update_part_is_legal(
    const T1Tex tex,
    const u32 x,
    const u32 y,
    const u32 w,
    const u32 h)
{
    const s32 array_i = T1_tex_to_array_i(tex);
    const s32 slice_i = T1_tex_to_slice_i(tex);
    if (
        array_i < 0 || array_i >= (s32)T1_tex_arrays_size ||
        slice_i < 0 || slice_i >= (s32)T1_tex_arrays[array_i].images_size ||
        T1_tex_arrays[array_i].images[slice_i].deleted)
    {
        return 0;
    }
    const u32 tex_w = T1_tex_arrays[array_i].single_img_width;
    const u32 tex_h = T1_tex_arrays[array_i].single_img_height;
    return
        w > 0 && h > 0 &&
        x <= tex_w && w <= tex_w - x &&
        y <= tex_h && h <= tex_h - y &&
        x % 4 == 0 && y % 4 == 0 &&
        (w % 4 == 0 || x + w == tex_w) &&
        (h % 4 == 0 || y + h == tex_h);
}

void T1_tex_update_from_rgba(
    T1Tex tex,
    u32 x,
    u32 y,
    u32 width,
    u32 height,
    const u8 * rgba)
{
    const b8 legal = rgba != NULL &&
        T1_tex_update_part_is_legal(tex, x, y, width, height);
    T1_assert(legal);
    if (!legal) { return; }
    const s32 array_i = T1_tex_to_array_i(tex);
    const s32 slice_i = T1_tex_to_slice_i(tex);
    
    const T1TexFormat format = T1_tex_arrays[array_i].format;
    if (format != T1_TEX_FORMAT_RGBA8) {
        // encoded first (no mipmaps: see T1TexFormat)
        u8 * blocks = T1_mem_malloc_managed(
            T1_tex_format_bytes(format, width, height));
        if (format == T1_TEX_FORMAT_BC3) {
            T1_bc3_encode(rgba, width, height, blocks);
        } else {
            T1_etc2_eac_encode(rgba, width, height, blocks);
        }
        T1_os_gpu_write_tex_slice(
            array_i, slice_i, x, y, width, height, blocks);
        T1_mem_free_managed(blocks);
        return;
    }
    
    // (the platform copies what it needs: rgba is the caller's again)
    T1_os_gpu_write_tex_slice(array_i, slice_i, x, y, width, height, rgba);
    
    #if T1_MIPMAPS_ACTIVE == T1_ACTIVE
    T1_os_gpu_generate_mipmaps_for_texture_array(array_i);
    #elif T1_MIPMAPS_ACTIVE == T1_INACTIVE
    #else
    #error "T1_MIPMAPS_ACTIVE undefined!"
    #endif
}

void T1_tex_update_from_bc3(
    T1Tex tex,
    u32 x,
    u32 y,
    u32 width,
    u32 height,
    const u8 * bc3)
{
    const b8 legal = bc3 != NULL &&
        T1_tex_update_part_is_legal(tex, x, y, width, height);
    T1_assert(legal);
    if (!legal) { return; }
    const s32 array_i = T1_tex_to_array_i(tex);
    
    // a BC3 texture: the blocks as they are
    if (T1_tex_arrays[array_i].format == T1_TEX_FORMAT_BC3) {
        T1_os_gpu_write_tex_slice(
            array_i, T1_tex_to_slice_i(tex), x, y, width, height, bc3);
        return;
    }
    
    // any other: their pixels
    u8 * rgba = T1_mem_malloc_managed((u64)width * height * 4);
    T1_bc_decode(bc3, width, height, /* bc3: */ 1, rgba);
    T1_tex_update_from_rgba(tex, x, y, width, height, rgba);
    T1_mem_free_managed(rgba);
}

void T1_tex_delete(T1Tex tex) {
    if (tex == T1_TEX_NONE) { return; }
    
    T1_tex_array_delete_slice(
        T1_tex_to_array_i(tex),
        T1_tex_to_slice_i(tex));
}

u32 T1_tex_get_width(T1Tex tex) {
    T1_assert(tex != T1_TEX_NONE);
    return T1_tex_arrays[T1_tex_to_array_i(tex)].single_img_width;
}

u32 T1_tex_get_height(T1Tex tex) {
    T1_assert(tex != T1_TEX_NONE);
    return T1_tex_arrays[T1_tex_to_array_i(tex)].single_img_height;
}
