#include "T1_tex_array.h"

#include "debigulator/src/decode_png.h"
#include "debigulator/src/decode_bmp.h"

#include "T1_log.h"
#include "T1_mem.h"
#include "T1_platform_layer.h"
// #include "T1_cpu_gpu_shared_types.h"
#include "T1_render_view.h"

static u32
tex_arrays_mutex_ids[T1_TEXARRAYS_CAP];

T1TexArray * T1_tex_arrays = NULL;
u32 T1_tex_arrays_size = 0;

void T1_tex_array_push_all(void)
{
    #if T1_TEXTURES_ACTIVE == T1_ACTIVE
    for (
        s32 ta_i = 1;
        ta_i < (s32)T1_tex_arrays_size;
        ta_i++)
    {
        for (
            s32 sl_i = (s32)T1_tex_arrays[ta_i].images_size - 1;
            sl_i >= 0;
            sl_i--)
        {
            if (
                T1_tex_arrays[ta_i].images[sl_i].image.rgba_values_freeable != NULL)
            {
                T1_os_gpu_push_tex_slice_and_free_rgba(
                    /* const s32 texture_array_i: */
                        ta_i,
                    /* const s32 texture_i: */
                        sl_i);
            }
        }
    }
    #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
}

static T1Img * extract_image(
    const T1Img * original,
    const u32 sprite_columns,
    const u32 sprite_rows,
    const u32 x,
    const u32 y)
{
    T1_log_assert(x > 0);
    T1_log_assert(y > 0);
    T1_log_assert(original != NULL);
    T1_log_assert(original->rgba_values_freeable != NULL);
    T1_log_assert(original->rgba_values_page_aligned != NULL);
    
    if (!T1_log_app_running) { return NULL; }
    T1_log_assert(sprite_columns > 0);
    T1_log_assert(sprite_rows > 0);
    T1_log_assert(x <= sprite_columns);
    T1_log_assert(y <= sprite_rows);
    if (!T1_log_app_running) { return NULL; }
    
    T1Img * new_image = T1_mem_malloc_unmanaged(sizeof(T1Img));
    T1_log_assert(new_image != NULL);
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    if (!T1_log_app_running) { return NULL; }
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
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
        /* const u64 subptr_size: */
            new_image->rgba_values_size);
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
        
        if (!T1_log_app_running) {
            new_image->good = false;
            break;
        }
        
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
    
    new_image->good = true;
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
    const char * filename_prefix,
    const s32 ta_i,
    const u32 rows,
    const u32 columns)
{
    T1_log_assert(new_image != NULL);
    if (new_image == NULL) { return; }
    T1_log_assert(new_image->rgba_values_freeable != NULL);
    T1_log_assert(new_image->rgba_values_size > 0);
    T1_log_assert(rows >= 1);
    T1_log_assert(columns >= 1);
    
    if (new_image->rgba_values_size < 1) { return; }
        
    char * filenames[256];
    for (u32 i = 0; i < 256; i++) {
        filenames[i] = T1_mem_malloc_managed(256);
        T1_std_memset(filenames[i], 0, sizeof(rows * columns * 256));
    }
    
    // each image should have the exact same dimensions
    u32 expected_width = new_image->width / columns;
    u32 expected_height = new_image->height / rows;
    T1_tex_arrays[ta_i].single_img_width  = expected_width;
    T1_tex_arrays[ta_i].single_img_height = expected_height;
    
    T1_tex_arrays[ta_i].images_size = rows * columns;
    T1_tex_arrays[ta_i].is_render_target = false;
    T1_tex_arrays[ta_i].bc1_compressed = false;
    
    for (s32 row_i = (s32)rows-1; row_i >= 0; row_i--) {
        for (s32 col_i = (s32)columns-1; col_i >= 0; col_i--)
        {
            s32 t_i = (row_i * ((s32)columns)) + col_i;
            
            T1Img * split_img = extract_image(
                /* const DecodedImage * original: */
                    new_image,
                /* const u32 sprite_columns: */
                    columns,
                /* const u32 sprite_rows: */
                    rows,
                /* const u32 x: */
                    (u32)col_i + 1,
                /* const u32 y: */
                    (u32)row_i + 1);
            
            if (split_img == NULL ||
                split_img->rgba_values_freeable == NULL ||
                split_img->rgba_values_page_aligned == NULL)
            {
                T1_log_assert(0);
                continue;
            }
            T1_log_assert(split_img->good);
            
            T1_log_assert(split_img->width  == expected_width);
            T1_log_assert(split_img->height == expected_height);
            
            T1_std_strcpy_cap(
                filenames[(row_i*(s32)columns)+col_i],
                256,
                filename_prefix);
            T1_std_strcat_cap(
                filenames[(row_i*(s32)columns)+col_i],
                256,
                "_");
            T1_std_strcat_s32_cap(
                filenames[(row_i*(s32)columns)+col_i],
                256,
                col_i);
            T1_std_strcat_cap(
                filenames[(row_i*(s32)columns)+col_i],
                256,
                "_");
            T1_std_strcat_s32_cap(
                filenames[(row_i*(s32)columns)+col_i],
                256,
                row_i);
            
            T1_std_strcpy_cap(
                T1_tex_arrays[ta_i].images[t_i].name,
                256,
                filenames[(row_i*(s32)columns)+col_i]);
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
            
            T1_os_gpu_push_tex_slice_and_free_rgba(
                /* const s32 texture_array_i: */
                    ta_i,
                /* const s32 texture_i: */
                    t_i);
        }
    }
    
    for (u32 i = 0; i < 256; i++) {
        T1_mem_free_managed(filenames[i]);
    }
}

s32 T1_tex_array_create_new_render_view(
    const u32 width,
    const u32 height)
{
    T1_log_assert(T1_render_views != NULL);
    
    s32 rv_i = T1_render_view_fetch_next(
        width,
        height);
    if (rv_i < 0) { return rv_i; }
    
    T1_render_views->cpu[rv_i].write_type =
        T1RENDERVIEW_WRITE_RENDER_TARGET;
    T1_log_assert(T1_render_views->cpu[rv_i].width ==
        width);
    T1_log_assert(T1_render_views->cpu[rv_i].height ==
        height);
    T1_render_views->cpu[rv_i].passes_size = 3;
    T1_render_views->cpu[rv_i].passes[0].type =
        T1RENDERPASS_DIAMOND_ALPHA;
    T1_render_views->cpu[rv_i].passes[1].type =
        T1RENDERPASS_ALPHA_BLEND;
    T1_render_views->cpu[rv_i].passes[2].type =
        T1RENDERPASS_FLAT_TEXQUADS;
    
    char tex_name[64];
    T1_std_memset(tex_name, 0, 64);
    T1_std_strcpy_cap(
        tex_name,
        64,
        "__%T1%__renderview_");
    T1_std_strcat_s32_cap(
        tex_name,
        64,
        rv_i);
    
    T1Tex existing = T1_tex_array_get_filename_loc(tex_name);
    
    T1_log_assert(existing == T1_TEX_NONE);
    
    T1Tex tex = T1_tex_array_reg_img(
        /* const char * filename: */
            tex_name,
        /* const u32 width: */
            T1_render_views->cpu[rv_i].width,
        /* const u32 height: */
            T1_render_views->cpu[rv_i].height,
        /* const u32 is_render_target: */
            true,
        /* const u32 use_bc1_compression: */
            false);
    
    T1_render_views->cpu[rv_i].write_tex = tex;
    T1_log_assert(T1_render_views->cpu[rv_i].write_tex != T1_TEX_NONE);
    
    T1_log_assert((s32)T1_tex_arrays[T1_tex_to_array_i(tex)].images_size > T1_tex_to_slice_i(tex));
    T1_log_assert(!T1_tex_arrays[T1_tex_to_array_i(tex)].deleted);
    T1_log_assert(!T1_tex_arrays[T1_tex_to_array_i(tex)].images[T1_tex_to_slice_i(tex)].deleted);
    T1_log_assert(T1_tex_arrays[T1_tex_to_array_i(tex)].images[T1_tex_to_slice_i(tex)].image.width == width);
    T1_log_assert(T1_tex_arrays[T1_tex_to_array_i(tex)].images[T1_tex_to_slice_i(tex)].image.height == height);
    
    T1_os_gpu_update_capacity_if_needed(T1_tex_to_array_i(tex));
    
    return rv_i;
}

b8 T1_tex_array_tex_exists_and_is_not_deleted(T1Tex in) {
    return
        in != T1_TEX_NONE &&
        (
        T1_tex_to_array_i(in) >= (s32)T1_tex_arrays_size ||
        T1_tex_to_slice_i(in) >= 
            (s32)T1_tex_arrays[T1_tex_to_array_i(in)].images_size ||
        T1_tex_arrays[T1_tex_to_array_i(in)].images[T1_tex_to_slice_i(in)].deleted
        );
}

u32 T1_tex_array_get_img_height(s32 array_i) {
    T1_log_assert(array_i < T1_tex_arrays_size);
    return T1_tex_arrays[array_i].single_img_height;
}

u32 T1_tex_array_get_img_width(s32 array_i) {
    T1_log_assert(array_i < T1_tex_arrays_size);
    return T1_tex_arrays[array_i].single_img_width;
}

void T1_tex_array_delete_array(
    const s32 array_i)
{
    T1_log_assert(array_i != 0);
    
    T1_os_gpu_delete_texture_array(array_i);
    
    T1_std_memset(
        T1_tex_arrays + array_i,
        0,
        sizeof(T1TexArray));
    T1_tex_arrays[array_i].deleted = true;
}

void T1_tex_array_delete_slice(
    const s32 array_i,
    const s32 slice_i)
{
    T1_log_assert(array_i >= 0);
    T1_log_assert(slice_i >= 0);
    if (array_i != T1_DEPTH_TEXTUREARRAYS_I) {
        T1_log_assert(
            slice_i < (s32)
                T1_tex_arrays[array_i].images_size);
        T1_log_assert(
            !T1_tex_arrays[array_i].images[slice_i].
                deleted);
    }
    
    if (array_i != T1_DEPTH_TEXTUREARRAYS_I) {
        T1_log_assert(
            array_i < (s32)T1_tex_arrays_size);
        
        T1_std_memset(
            T1_tex_arrays[array_i].images + slice_i,
            0,
            sizeof(T1TexArrayImg));
        T1_tex_arrays[array_i].images[slice_i].
            deleted = true;
        
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
    } else {
        T1_os_gpu_delete_depth_tex(slice_i);
    }
}

void T1_tex_array_update_rgba(
    const s32 array_i,
    const s32 slice_i,
    const u8 * rgba,
    const u32 rgba_size)
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
            /* const u64 subptr_size: */
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
    const char * filename_prefix,
    const u32 rows,
    const u32 columns)
{
    T1_log_assert(new_image != NULL);
    if (new_image == NULL) { return; }
    
    s32 new_texture_array_i = (int)T1_tex_arrays_size;
    T1_tex_arrays[new_texture_array_i].request_init = true;
    T1_tex_arrays_size++;
    T1_log_assert(T1_tex_arrays_size <= T1_TEXARRAYS_CAP);
    
    register_to_texturearray_by_splitting_image(
        /* DecodedImage * new_image: */
            new_image,
        /* filename_prefix: */
            filename_prefix,
        /* const s32 texture_array_i: */
            new_texture_array_i,
        /* const u32 rows: */
            rows,
        /* const u32 columns: */
            columns);
}

T1Tex T1_tex_array_reg_img(
    const c8 * filename,
    const u32 width,
    const u32 height,
    const b8 is_render_target,
    const b8 use_bc1_compression)
{
    T1Tex retval = T1_TEX_NONE;
    
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
        T1_log_assert(i < T1_TEXARRAYS_CAP);
        if (
            !T1_tex_arrays[i].deleted &&
            T1_tex_arrays[i].images_size > 0 &&
            T1_tex_arrays[i].single_img_width  == width &&
            T1_tex_arrays[i].single_img_height == height &&
            T1_tex_arrays[i].is_render_target == is_render_target &&
            T1_tex_arrays[i].bc1_compressed == use_bc1_compression)
        {
            const s16 slice_i = (s16)T1_tex_arrays[i].images_size;
            T1_tex_set_array_i(&retval, i);
            T1_tex_set_slice_i(&retval, slice_i); 
            T1_tex_arrays[i].images_size++;
            
            T1_tex_arrays[i].images[slice_i].image.width =
                width;
            T1_tex_arrays[i].images[slice_i].image.height =
                height;
            T1_tex_arrays[i].images[slice_i].image.rgba_values_size =
                width * height * 4;
            T1_tex_arrays[i].images[slice_i].image.good = 1;
            
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
            T1_tex_arrays_size++;
        }
        
        T1_log_assert(T1_tex_arrays_size <= T1_TEXARRAYS_CAP);
        T1_tex_set_array_i(&retval, array_i);
        T1_tex_set_slice_i(&retval, 0);
        
        T1_tex_arrays[array_i].deleted = 0;
        T1_tex_arrays[array_i].images_size = 1;
        T1_tex_arrays[array_i].single_img_width = width;
        T1_tex_arrays[array_i].single_img_height = height;
        T1_tex_arrays[array_i].is_render_target = is_render_target > 0;
        T1_tex_arrays[array_i].bc1_compressed =
            use_bc1_compression > 0;
        T1_tex_arrays[array_i].images[0].image.width = width;
        T1_tex_arrays[array_i].images[0].image.height =
            height;
        T1_tex_arrays[array_i].images[0].image.rgba_values_size =
            width * height * 4;
        T1_tex_arrays[array_i].images[0].image.good = 1;
    } else {
        T1_log_assert(T1_tex_to_slice_i(retval) > 0);
    }
    
    T1_log_assert(retval != T1_TEX_NONE);
    T1_std_strcpy_cap(
        T1_tex_arrays[T1_tex_to_array_i(retval)].
            images[T1_tex_to_slice_i(retval)].name,
        T1_TEX_NAME_CAP,
        filename);
    
    return retval;
}

T1Tex T1_tex_array_get_filename_loc(
    const char * for_filename)
{
    // T1_log_assert(for_filename != NULL);
    // T1_log_assert(for_filename[0] != '\0');
        
    if (for_filename == NULL || for_filename[0] == '\0') {
        return T1_TEX_NONE;
    }
    
    for (s16 i = 0; i < (s16)T1_tex_arrays_size; i++) {
        T1_log_assert(T1_tex_arrays[i].images_size < 2000);
        for (s16 j = 0; j < (s16)T1_tex_arrays[i].images_size; j++) {
            if (
                T1_std_are_equal_strings(
                    T1_tex_arrays[i].images[j].name,
                    for_filename))
            {
                T1Tex return_value;
                T1_tex_set_array_i(&return_value, i);
                T1_tex_set_slice_i(&return_value, j);
                return return_value;
            }
        }
    }
    
    return T1_TEX_NONE;
}

void T1_tex_array_debug_dump_to_writables(
    const s32 texture_array_i,
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
            /* const s32 texture_array_i: */
                texture_array_i,
            /* const s32 texture_i: */
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
                *success = false;
                return;
            }
            continue;
        }
        
        if (rgba_size < 4) {
            *success = false;
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
            /* const u32 rgba_size: */
                rgba_size,
            /* const u32 width: */
                width,
            /* const u32 height: */
                height,
            /* u32 * good: */
                &write_good);
        
        T1_mem_free_managed(rgba);
        
        if (!write_good) {
            *success = false;
            return;
        }
    }
    
    *success = 1;
    #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
}
