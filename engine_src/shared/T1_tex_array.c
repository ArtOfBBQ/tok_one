#include "T1_tex_array.h"

#include "T1_log.h"
#include "T1_mem.h"
#include "T1_os.h"
// #include "T1_cpu_gpu_shared_types.h"
#include "T1_render_view.h"

static u32 tex_arrays_mutex_ids[T1_TEXARRAYS_CAP];

T1TexArray * T1_tex_arrays = NULL;
u32 T1_tex_arrays_size = 0;

static T1Img * extract_image(
    const T1Img * original,
    u32 sprite_columns,
    u32 sprite_rows,
    u32 x,
    u32 y,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return NULL; }
    
    T1_log_assert(x > 0);
    T1_log_assert(y > 0);
    T1_log_assert(original != NULL);
    if (!original->never_delete_me) {
        T1_log_assert(original->rgba_values_freeable != NULL);
    } else {
        T1_log_assert(original->rgba_values_freeable == NULL);
    }
    T1_log_assert(original->rgba_values_page_aligned != NULL);
    
    T1_log_assert(sprite_columns > 0);
    T1_log_assert(sprite_rows > 0);
    T1_log_assert(x <= sprite_columns);
    T1_log_assert(y <= sprite_rows);
    
    T1Img * new_image = T1_mem_malloc_unmanaged(sizeof(T1Img));
    if (!new_image) {
        *fatal_error = "Malloc fail";
        return NULL;
    }
    
    u32 slice_size_bytes =
        (original->rgba_values_size
            / sprite_columns)
            / sprite_rows;
    u32 slice_width_pixels =
        original->width / sprite_columns;
    u32 slice_height_pixels = original->height / sprite_rows;
    T1_log_assert(slice_size_bytes > 0);
    T1_log_assert(slice_width_pixels > 0);
    T1_log_assert(slice_height_pixels > 0);
    T1_log_assert(
        slice_size_bytes == slice_width_pixels * slice_height_pixels * 4);
    T1_log_assert(
        slice_size_bytes *
            sprite_columns *
            sprite_rows ==
                original->rgba_values_size);
    
    new_image->rgba_values_size = slice_size_bytes;
    T1_mem_malloc_managed_page_aligned(
        /* void * base_pointer_for_freeing: */
            (void *)&new_image->rgba_values_freeable,
        /* void * aligned_subptr: */
            (void *)&new_image->rgba_values_page_aligned,
        /* u64 subptr_size: */
            new_image->rgba_values_size);
    if (!new_image->rgba_values_page_aligned) {
        *fatal_error = "Page-aligned malloc failure";
        return NULL;
    }
    T1_std_memset(
        new_image->rgba_values_page_aligned,
        0,
        new_image->rgba_values_size);
    
    new_image->width = slice_width_pixels;
    new_image->height = slice_height_pixels;
    
    u32 start_x_pixels = 1 + ((x - 1) * slice_width_pixels);
    u32 start_y_pixels = 1 + ((y - 1) * slice_height_pixels);
    u32 end_y_pixels = start_y_pixels + slice_height_pixels - 1;
    
    u32 i = 0;
    for (
        u32 cur_y_pixels = start_y_pixels;
        cur_y_pixels <= end_y_pixels;
        cur_y_pixels++)
    {
        // get the pixel that's at [start_x_pixels, cur_y_pixels]
        // copcur_y slice_width pixels
        u64 rgba_value_i =
            ((start_x_pixels - 1) * 4)
                + ((cur_y_pixels - 1) * original->width * 4);
        
        for (
            u32 _ = 0;
            _ < (slice_width_pixels * 4);
            _++)
        {
            T1_log_assert((rgba_value_i + _) < original->rgba_values_size);
            T1_log_assert(i < new_image->rgba_values_size);
            new_image->rgba_values_page_aligned[i] =
                original->rgba_values_page_aligned[rgba_value_i + _];
            i++;
        }
    }
    
    return new_image;
}

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

static void
register_to_texturearray_by_splitting_image(
    T1Img * new_image,
    s32 ta_i,
    u32 rows,
    u32 columns,
    b8 never_delete,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    T1_log_assert(new_image != NULL);
    if (new_image == NULL) { return; }
    T1_log_assert(new_image->rgba_values_page_aligned != NULL);
    T1_log_assert(new_image->rgba_values_size > 0);
    T1_log_assert(rows >= 1);
    T1_log_assert(columns >= 1);
    
    if (new_image->rgba_values_size < 1) {
        *fatal_error = "register_to_texturearray_by_splitting_image() but rgba_values_size was 0";
        return;
    }
        
    // each image should have the exact same dimensions
    u32 expected_width = new_image->width / columns;
    u32 expected_height = new_image->height / rows;
    T1_tex_arrays[ta_i].single_img_width  = expected_width;
    T1_tex_arrays[ta_i].single_img_height = expected_height;
    
    T1_tex_arrays[ta_i].images_size = rows * columns;
    
    for (s32 row_i = (s32)rows-1; row_i >= 0; row_i--) {
        for (s32 col_i = (s32)columns-1; col_i >= 0; col_i--)
        {
            s32 t_i = (row_i * ((s32)columns)) + col_i;
            
            T1Img * split_img = extract_image(
                /* const DecodedImage * original: */
                    new_image,
                /* u32 sprite_columns: */
                    columns,
                /* u32 sprite_rows: */
                    rows,
                /* u32 x: */
                    (u32)col_i + 1,
                /* u32 y: */
                    (u32)row_i + 1,
                    fatal_error);
            
            if (*fatal_error != 0) {
                return;
            }
            
            if (split_img == NULL ||
                split_img->rgba_values_freeable == NULL ||
                split_img->rgba_values_page_aligned == NULL)
            {
                *fatal_error = "register_to_texturearray_by_splitting_image() failed to split image";
                return;
            }
            
            split_img->never_delete_me = never_delete;
            if (split_img->never_delete_me) {
                split_img->rgba_values_freeable = NULL;
            }
            
            T1_log_assert(split_img->width  == expected_width);
            T1_log_assert(split_img->height == expected_height);
            
            T1_tex_arrays[ta_i].images[t_i].image = *split_img;
            T1_tex_arrays[ta_i].images[t_i].image.width =
                split_img->width;
            T1_tex_arrays[ta_i].images[t_i].image.height =
                split_img->height;
            T1_tex_arrays[ta_i].images[t_i].image.pixel_count =
                split_img->width * split_img->height;
            T1_tex_arrays[ta_i].images[t_i].image.rgba_values_size =
                split_img->rgba_values_size;
            T1_tex_arrays[ta_i].images[t_i].image.rgba_values_freeable =
                split_img->rgba_values_freeable;
            T1_tex_arrays[ta_i].images[t_i].image.rgba_values_page_aligned =
                split_img->rgba_values_page_aligned;
            
            T1_os_gpu_push_tex_slice(
                /* s32 texture_array_i: */
                    ta_i,
                /* s32 texture_i: */
                    t_i);
        }
    }
}

u32 T1_tex_array_get_img_height(s32 array_i) {
    T1_log_assert((u32)array_i < T1_tex_arrays_size);
    return T1_tex_arrays[array_i].single_img_height;
}

u32 T1_tex_array_get_img_width(s32 array_i) {
    T1_log_assert((u32)array_i < T1_tex_arrays_size);
    return T1_tex_arrays[array_i].single_img_width;
}

void T1_tex_array_delete_array(
    s32 array_i)
{
    T1_log_assert(array_i != 0);
    for (u32 slice_i = 0; slice_i < T1_tex_arrays[array_i].images_size; slice_i++) {
        T1_log_assert(!T1_tex_arrays[array_i].images[slice_i].image.never_delete_me);
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
    T1_log_assert(array_i >= 0);
    T1_log_assert(slice_i >= 0);
    
    T1_log_assert(
        slice_i < (s32)
            T1_tex_arrays[array_i].images_size);
    T1_log_assert(
        !T1_tex_arrays[array_i].images[slice_i].
            deleted);
    
    if (T1_tex_arrays[array_i].images[slice_i].image.never_delete_me) {
        return;
    }
    
    T1_log_assert(
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

const u8 * T1_tex_array_get_const_rgba(
    s32 array_i,
    s32 slice_i)
{
    T1_log_assert(array_i < (s32)T1_tex_arrays_size);
    T1_log_assert(array_i < T1_TEXARRAYS_CAP);
    T1_log_assert(slice_i < (s32)T1_tex_arrays[array_i].images_size);
    
    return T1_tex_arrays[array_i].
        images[slice_i].image.rgba_values_page_aligned;
}

void T1_tex_array_update_rgba(
    s32 array_i,
    s32 slice_i,
    const u8 * rgba,
    u32 rgba_size)
{
    T1_log_assert(T1_tex_arrays[array_i].single_img_width * T1_tex_arrays[array_i].single_img_height * 4 == rgba_size);
    
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

void T1_tex_array_reg_new_by_splitting_img(
    T1Img * new_image,
    u32 rows,
    u32 columns,
    b8 never_delete,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    T1_log_assert(new_image != NULL);
    if (new_image == NULL) { return; }
    
    s32 new_texture_array_i = (int)T1_tex_arrays_size;
    T1_tex_arrays_size++;
    T1_log_assert(T1_tex_arrays_size <= T1_TEXARRAYS_CAP);
    
    register_to_texturearray_by_splitting_image(
        /* DecodedImage * new_img: */ new_image,
        /* s32 texture_array_i: */ new_texture_array_i,
        /* u32 rows: */ rows,
        /* u32 columns: */ columns,
        /* b8 free_rgba: */ never_delete,
        fatal_error);
}

T1Tex T1_tex_array_new_slice(
    u32 width,
    u32 height,
    b8 never_delete,
    char ** const fatal_error)
{
    T1Tex retval = T1_TEX_NONE;
    if (*fatal_error != 0) { return retval; }
    
    T1_log_assert(width > 0);
    T1_log_assert(height > 0);
    
    // *****************
    // find out if same width/height was already used
    // set new_texturearray_i and new_texture_i if so
    for (
        s16 i = 0;
        i < (s16)T1_tex_arrays_size;
        i++)
    {
        if (*fatal_error != 0) { return retval; }
        
        T1_log_assert(i < T1_TEXARRAYS_CAP);
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
            
            T1_log_assert(slice_i >= 1);
            T1_log_assert(slice_i < T1_TEX_SLICES_CAP);
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
        
        T1_log_assert(T1_tex_arrays_size <= T1_TEXARRAYS_CAP);
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
        T1_log_assert(T1_tex_to_slice_i(retval) > 0);
    }
    
    T1_log_assert(retval != T1_TEX_NONE);
    
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
    T1_log_assert(array_i >= 0);
    T1_log_assert(array_i < (s32)T1_tex_arrays_size);
    T1_log_assert(slice_i >= 0);
    T1_log_assert(slice_i < (s32)T1_tex_arrays[array_i].images_size);
    T1_log_assert(!T1_tex_arrays[array_i].images[slice_i].deleted);
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
    T1_log_assert(tex != T1_TEX_NONE);
    return T1_tex_arrays[T1_tex_to_array_i(tex)].single_img_width;
}

u32 T1_tex_get_height(T1Tex tex) {
    T1_log_assert(tex != T1_TEX_NONE);
    return T1_tex_arrays[T1_tex_to_array_i(tex)].single_img_height;
}

void T1_tex_array_debug_dump_to_writables(
    s32 texture_array_i,
    u32 * success)
{
    #if T1_TEXTURES_ACTIVE == T1_ACTIVE
    for (s32 texture_i = 0; texture_i < 100; texture_i++) {
        u32 fetched = 0;
        
        u32 rgba_cap = 30000000;
        u8 * rgba = T1_mem_malloc_managed(rgba_cap);
        u32 rgba_size = 0;
        u32 width = 0;
        u32 height = 0;
        
        T1_os_gpu_fetch_rgba_at(
            /* s32 texture_array_i: */
                texture_array_i,
            /* s32 texture_i: */
                texture_i,
            /* u8 *rgba_recipient: */
                rgba,
            /* u32 * recipient_size: */
                &rgba_size,
            /* u32 * recipient_width: */
                &width,
            /* u32 * recipient_height: */
                &height,
            /* u32 recipient_cap: */
                rgba_cap,
            /* u32 *good: */
                &fetched);
        
        if (!fetched) {
            if (texture_i == 0) {
                *success = 0;
                return;
            }
            continue;
        }
        
        if (rgba_size < 4) {
            *success = 0;
            return;
        }
        
        char filename[128];
        T1_std_strcpy_cap(filename, 128, "dumped_texturearray_");
        T1_std_strcat_s32_cap(filename, 128, texture_array_i);
        T1_std_strcat_cap(filename, 128, "_");
        T1_std_strcat_s32_cap(filename, 128, texture_i);
        T1_std_strcat_cap(filename, 128, ".bmp");
        
        u8 write_good = 0;
        T1_os_write_rgba_to_writables(
            /* const char * local_filename: */
                filename,
            /* u8 * rgba: */
                rgba,
            /* u32 rgba_size: */
                rgba_size,
            /* u32 width: */
                width,
            /* u32 height: */
                height,
            /* u32 * good: */
                &write_good);
        
        T1_mem_free_managed(rgba);
        
        if (!write_good) {
            *success = 0;
            return;
        }
    }
    
    *success = 1;
    #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
}
