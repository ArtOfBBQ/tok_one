#include "T1_tex_array.h"

#include "T1_assert.h"
#include "T1_mem.h"
#include "T1_os.h"
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
    
    if (T1_tex_arrays[array_i].images[slice_i].image.
        rgba_values_freeable != NULL)
    {
        T1_mem_free_managed(
            T1_tex_arrays[array_i].images[slice_i].image.
                rgba_values_freeable);
    }
    
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

void T1_tex_array_update_rgba(
    s32 array_i,
    s32 slice_i,
    const u8 * rgba,
    u32 rgba_size)
{
    T1_assert(T1_tex_arrays[array_i].single_img_width * T1_tex_arrays[array_i].single_img_height * 4 == rgba_size);
    
    T1_tex_arrays[array_i].images[slice_i].image.
        rgba_values_size = rgba_size;
    if (
        T1_tex_arrays[array_i].
            images[slice_i].image.
                rgba_values_freeable == NULL)
    {
        T1_mem_malloc_managed_page_aligned(
            /* void **base_pointer_for_freeing: */
                (void **)&T1_tex_arrays[array_i].
                    images[slice_i].image.
                        rgba_values_freeable,
            /* void **aligned_subptr: */
                (void **)&T1_tex_arrays[array_i].
                    images[slice_i].image.
                        rgba_values_page_aligned,
            /* u64 subptr_size: */
                rgba_size);
    }
    T1_std_memcpy(
        T1_tex_arrays[array_i].images[slice_i].
            image.rgba_values_page_aligned,
        rgba,
        rgba_size);
}

T1Tex T1_tex_array_new_slice(
    u32 width,
    u32 height,
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
            T1_tex_arrays[i].single_img_height == height)
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
    const u8 * rgba,
    u32 width,
    u32 height,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return T1_TEX_NONE; }
    
    if (width < 1 || height < 1) {
        *fatal_error = "T1_tex_new() needs a width and height";
        return T1_TEX_NONE;
    }
    
    T1Tex tex = T1_tex_array_new_slice(
        width,
        height,
        /* b8 never_delete: */ 0,
        fatal_error);
    if (*fatal_error != 0) { return T1_TEX_NONE; }
    
    if (rgba != NULL) {
        T1_tex_update(tex, rgba);
    } else {
        // blank until a pass draws to it
        T1_os_gpu_update_capacity_if_needed(T1_tex_to_array_i(tex));
    }
    
    return tex;
}

void T1_tex_update(
    T1Tex tex,
    const u8 * rgba)
{
    s32 array_i = T1_tex_to_array_i(tex);
    s32 slice_i = T1_tex_to_slice_i(tex);
    T1_assert(array_i >= 0);
    T1_assert(array_i < (s32)T1_tex_arrays_size);
    T1_assert(slice_i >= 0);
    T1_assert(slice_i < (s32)T1_tex_arrays[array_i].images_size);
    T1_assert(!T1_tex_arrays[array_i].images[slice_i].deleted);
    T1_tex_array_update_rgba(
        array_i,
        slice_i,
        rgba,
        T1_tex_arrays[array_i].single_img_width *
            T1_tex_arrays[array_i].single_img_height * 4);
    
    T1_os_gpu_push_tex_slice(array_i, slice_i);

    #if T1_MIPMAPS_ACTIVE == T1_ACTIVE
    T1_os_gpu_generate_mipmaps_for_texture_array(array_i);
    #elif T1_MIPMAPS_ACTIVE == T1_INACTIVE
    #else
    #error "T1_MIPMAPS_ACTIVE undefined!"
    #endif
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
