#include "T1_tex_array.h"


static uint32_t
tex_arrays_mutex_ids[T1_TEXARRAYS_CAP];

T1TexArray * T1_tex_arrays = NULL;
uint32_t T1_tex_arrays_size = 0;

void T1_tex_array_push_all(void)
{
    #if T1_TEXTURES_ACTIVE == T1_ACTIVE
    for (
        int32_t ta_i = 1;
        ta_i < (int32_t)T1_tex_arrays_size;
        ta_i++)
    {
        for (
            int32_t sl_i = (int32_t)T1_tex_arrays[ta_i].images_size - 1;
            sl_i >= 0;
            sl_i--)
        {
            if (
                T1_tex_arrays[ta_i].images[sl_i].image.rgba_values_freeable != NULL)
            {
                T1_platform_gpu_push_tex_slice_and_free_rgba(
                    /* const int32_t texture_array_i: */
                        ta_i,
                    /* const int32_t texture_i: */
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
    const uint32_t sprite_columns,
    const uint32_t sprite_rows,
    const uint32_t x,
    const uint32_t y)
{
    T1_log_assert(x > 0);
    T1_log_assert(y > 0);
    T1_log_assert(original != NULL);
    T1_log_assert(original->rgba_values_freeable != NULL);
    T1_log_assert(original->rgba_values_page_aligned != NULL);
    
    if (!T1_logger_app_running) { return NULL; }
    T1_log_assert(sprite_columns > 0);
    T1_log_assert(sprite_rows > 0);
    T1_log_assert(x <= sprite_columns);
    T1_log_assert(y <= sprite_rows);
    if (!T1_logger_app_running) { return NULL; }
    
    T1Img * new_image = T1_mem_malloc_unmanaged(sizeof(T1Img));
    T1_log_assert(new_image != NULL);
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    if (!T1_logger_app_running) { return NULL; }
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    uint32_t slice_size_bytes =
        (original->rgba_values_size
            / sprite_columns)
            / sprite_rows;
    uint32_t slice_width_pixels =
        original->width / sprite_columns;
    uint32_t slice_height_pixels = original->height / sprite_rows;
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
        /* const size_t subptr_size: */
            new_image->rgba_values_size);
    T1_std_memset(
        new_image->rgba_values_page_aligned,
        0,
        new_image->rgba_values_size);
    
    new_image->width = slice_width_pixels;
    new_image->height = slice_height_pixels;
    
    uint32_t start_x_pixels = 1 + ((x - 1) * slice_width_pixels);
    uint32_t start_y_pixels = 1 + ((y - 1) * slice_height_pixels);
    uint32_t end_y_pixels = start_y_pixels + slice_height_pixels - 1;
    
    uint32_t i = 0;
    for (
        uint32_t cur_y_pixels = start_y_pixels;
        cur_y_pixels <= end_y_pixels;
        cur_y_pixels++)
    {
        // get the pixel that's at [start_x_pixels, cur_y_pixels]
        // copcur_y slice_width pixels
        uint64_t rgba_value_i =
            ((start_x_pixels - 1) * 4)
                + ((cur_y_pixels - 1) * original->width * 4);
        
        if (!T1_logger_app_running) {
            new_image->good = false;
            break;
        }
        
        for (
            uint32_t _ = 0;
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
    
    for (uint32_t i = 0; i < T1_TEXARRAYS_CAP; i++)
    {
        tex_arrays_mutex_ids[i] = T1_platform_init_mutex_and_return_id();
    }
}

static void
register_to_texturearray_by_splitting_image(
    T1Img * new_image,
    const char * filename_prefix,
    const int32_t ta_i,
    const uint32_t rows,
    const uint32_t columns)
{
    T1_log_assert(new_image != NULL);
    if (new_image == NULL) { return; }
    T1_log_assert(new_image->rgba_values_freeable != NULL);
    T1_log_assert(new_image->rgba_values_size > 0);
    T1_log_assert(rows >= 1);
    T1_log_assert(columns >= 1);
    
    if (new_image->rgba_values_size < 1) { return; }
        
    char * filenames[256];
    for (uint32_t i = 0; i < 256; i++) {
        filenames[i] = T1_mem_malloc_managed(256);
        T1_std_memset(filenames[i], 0, sizeof(rows * columns * 256));
    }
    
    // each image should have the exact same dimensions
    uint32_t expected_width = new_image->width / columns;
    uint32_t expected_height = new_image->height / rows;
    T1_tex_arrays[ta_i].single_img_width  = expected_width;
    T1_tex_arrays[ta_i].single_img_height = expected_height;
    
    #if 0
    T1_platform_gpu_copy_texture_array(
        /* const int32_t texture_array_i: */
            ta_i,
        /* const uint32_t num_images: */
            rows * columns,
        /* const uint32_t single_image_width: */
            T1_tex_arrays[ta_i].single_img_width,
        /* const uint32_t single_image_height: */
            T1_tex_arrays[ta_i].single_img_height,
        /* const bool32_t is_render_target: */
            false,
        /* const uint32_t use_bc1_compression: */
            false);
    #endif
    
    T1_tex_arrays[ta_i].images_size = rows * columns;
    T1_tex_arrays[ta_i].is_render_target = false;
    T1_tex_arrays[ta_i].bc1_compressed = false;
    
    for (int32_t row_i = (int32_t)rows-1; row_i >= 0; row_i--) {
        for (int32_t col_i = (int32_t)columns-1; col_i >= 0; col_i--)
        {
            int32_t t_i = (row_i * ((int32_t)columns)) + col_i;
            
            T1Img * split_img = extract_image(
                /* const DecodedImage * original: */
                    new_image,
                /* const uint32_t sprite_columns: */
                    columns,
                /* const uint32_t sprite_rows: */
                    rows,
                /* const uint32_t x: */
                    (uint32_t)col_i + 1,
                /* const uint32_t y: */
                    (uint32_t)row_i + 1);
            
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
                filenames[(row_i*(int32_t)columns)+col_i],
                256,
                filename_prefix);
            T1_std_strcat_cap(
                filenames[(row_i*(int32_t)columns)+col_i],
                256,
                "_");
            T1_std_strcat_int_cap(
                filenames[(row_i*(int32_t)columns)+col_i],
                256,
                col_i);
            T1_std_strcat_cap(
                filenames[(row_i*(int32_t)columns)+col_i],
                256,
                "_");
            T1_std_strcat_int_cap(
                filenames[(row_i*(int32_t)columns)+col_i],
                256,
                row_i);
            
            T1_std_strcpy_cap(
                T1_tex_arrays[ta_i].images[t_i].name,
                FILENAME_MAX,
                filenames[(row_i*(int32_t)columns)+col_i]);
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
            
            T1_platform_gpu_push_tex_slice_and_free_rgba(
                /* const int32_t texture_array_i: */
                    ta_i,
                /* const int32_t texture_i: */
                    t_i);
        }
    }
    
    for (uint32_t i = 0; i < 256; i++) {
        free(filenames[i]);
    }
}

int32_t T1_tex_array_create_new_render_view(
    const uint32_t width,
    const uint32_t height)
{
    T1_log_assert(T1_render_views != NULL);
    
    int32_t rv_i = T1_render_view_fetch_next(
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
    T1_std_strcpy_cap(
        tex_name,
        64,
        "__%T1%__renderview_");
    T1_std_strcat_int_cap(
        tex_name,
        64,
        rv_i);
    
    T1Tex existing =
        T1_tex_array_get_filename_loc(
            tex_name);
    
    T1_log_assert(existing.array_i < 0);
    T1_log_assert(existing.slice_i < 0);
    
    T1_tex_array_prereg_null_img(
        /* const char * filename: */
            tex_name,
        /* const uint32_t width: */
            T1_render_views->cpu[rv_i].width,
        /* const uint32_t height: */
            T1_render_views->cpu[rv_i].height,
        /* const uint32_t is_render_target: */
            true,
        /* const uint32_t use_bc1_compression: */
            false);
    
    T1Tex tex =
        T1_tex_array_get_filename_loc(
            tex_name);
    
    T1_render_views->cpu[rv_i].write_array_i =
        tex.array_i;
    T1_render_views->cpu[rv_i].write_slice_i =
        tex.slice_i;
    T1_log_assert(
        T1_render_views->cpu[rv_i].write_array_i >= 1);
    T1_log_assert(
        T1_render_views->cpu[rv_i].write_slice_i >= 0);
    
    T1_log_assert((int32_t)T1_tex_arrays[tex.array_i].images_size > tex.slice_i);
    T1_log_assert(!T1_tex_arrays[tex.array_i].deleted);
    T1_log_assert(!T1_tex_arrays[tex.array_i].images[tex.slice_i].deleted);
    T1_log_assert(T1_tex_arrays[tex.array_i].images[tex.slice_i].image.width == width);
    T1_log_assert(T1_tex_arrays[tex.array_i].images[tex.slice_i].image.height == height);
    
    T1_platform_gpu_update_capacity_if_needed(tex.array_i);
    
    return rv_i;
}

void T1_tex_array_delete_array(
    const int32_t array_i)
{
    T1_log_assert(array_i != 0);
    
    T1_platform_gpu_delete_texture_array(array_i);
    
    T1_std_memset(
        T1_tex_arrays + array_i,
        0,
        sizeof(T1TexArray));
    T1_tex_arrays[array_i].deleted = true;
}

void T1_tex_array_delete_slice(
    const int32_t array_i,
    const int32_t slice_i)
{
    T1_log_assert(array_i >= 0);
    T1_log_assert(slice_i >= 0);
    if (array_i != DEPTH_TEXTUREARRAYS_I) {
        T1_log_assert(
            slice_i < (int32_t)
                T1_tex_arrays[array_i].images_size);
        T1_log_assert(
            !T1_tex_arrays[array_i].images[slice_i].
                deleted);
    }
    
    if (array_i != DEPTH_TEXTUREARRAYS_I) {
        T1_log_assert(
            array_i < (int32_t)T1_tex_arrays_size);
        
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
        T1_platform_gpu_delete_depth_tex(slice_i);
    }
}

T1Tex T1_tex_array_reg_new_from_rgba(
    const char * fake_filename,
    const uint8_t * rgba,
    const uint32_t rgba_size,
    const uint32_t width,
    const uint32_t height)
{
    T1_log_assert(width * height * 4 == rgba_size);
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    T1Tex retval = T1_tex_array_get_filename_loc(fake_filename);
    T1_log_assert(retval.array_i < 0);
    T1_log_assert(retval.slice_i < 0);
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    for (int16_t i = 0; i < (int16_t)T1_tex_arrays_size; i++) {
        if (
            T1_tex_arrays[i].single_img_width  == width &&
            T1_tex_arrays[i].single_img_height == height &&
            !T1_tex_arrays[i].is_render_target &&
            !T1_tex_arrays[i].bc1_compressed)
        {
            retval.array_i = i;
            retval.slice_i = (int16_t)T1_tex_arrays[i].images_size;
            T1_tex_arrays[i].images_size += 1;
            break;
        }
    }
    
    if (retval.array_i < 0) {
        retval.array_i = (int16_t)T1_tex_arrays_size;
        retval.slice_i = 0;
        T1_tex_arrays_size += 1;
        T1_log_assert(T1_tex_arrays_size <= T1_TEXARRAYS_CAP);
        
        T1_tex_arrays[retval.array_i].images_size = 1;
        T1_tex_arrays[retval.array_i].bc1_compressed = false;
        T1_tex_arrays[retval.array_i].single_img_width  = width;
        T1_tex_arrays[retval.array_i].single_img_height = height;
        T1_tex_arrays[retval.array_i].deleted = false;
        T1_tex_arrays[retval.array_i].is_render_target = false;
        T1_tex_arrays[retval.array_i].request_init = 0;
    }
    
    T1_log_assert(retval.array_i >= 0);
    T1_log_assert(retval.slice_i >= 0);
    T1_log_assert(retval.array_i < T1_TEXARRAYS_CAP);
    T1_log_assert(retval.slice_i < T1_TEX_SLICES_CAP);
    
    T1_std_strcpy_cap(
        T1_tex_arrays[retval.array_i].images[retval.slice_i].name,
        T1_TEX_NAME_CAP,
        fake_filename);
    
    T1_tex_arrays[retval.array_i].images[retval.slice_i].image.rgba_values_size =
        rgba_size;
    if (
        T1_tex_arrays[retval.array_i].
            images[retval.slice_i].image.
                rgba_values_freeable == NULL)
    {
        T1_mem_malloc_managed_page_aligned(
            /* void **base_pointer_for_freeing: */
                (void **)&T1_tex_arrays[retval.array_i].
                    images[retval.slice_i].image.
                        rgba_values_freeable,
            /* void **aligned_subptr: */
                (void **)&T1_tex_arrays[retval.array_i].
                    images[retval.slice_i].image.
                        rgba_values_page_aligned,
            /* const size_t subptr_size: */
                rgba_size);
    }
    T1_std_memcpy(
        T1_tex_arrays[retval.array_i].images[retval.slice_i].
            image.rgba_values_page_aligned,
        rgba,
        rgba_size);
    
    return retval;
}

void T1_tex_array_reg_new_by_splitting_img(
    T1Img * new_image,
    const char * filename_prefix,
    const uint32_t rows,
    const uint32_t columns)
{
    T1_log_assert(new_image != NULL);
    if (new_image == NULL) { return; }
    
    int32_t new_texture_array_i = (int)T1_tex_arrays_size;
    T1_tex_arrays[new_texture_array_i].request_init = true;
    T1_tex_arrays_size++;
    T1_log_assert(T1_tex_arrays_size <= T1_TEXARRAYS_CAP);
    
    register_to_texturearray_by_splitting_image(
        /* DecodedImage * new_image: */
            new_image,
        /* filename_prefix: */
            filename_prefix,
        /* const int32_t texture_array_i: */
            new_texture_array_i,
        /* const uint32_t rows: */
            rows,
        /* const uint32_t columns: */
            columns);
}

void T1_tex_array_prereg_null_img(
    const char * filename,
    const uint32_t width,
    const uint32_t height,
    const uint32_t is_render_target,
    const uint32_t use_bc1_compression)
{
    int32_t new_texturearray_i = -1;
    int32_t new_texture_i = -1;
    
    T1_log_assert(width > 0);
    T1_log_assert(height > 0);
    
    // *****************
    // find out if same width/height was already used
    // set new_texturearray_i and new_texture_i if so
    for (
        uint32_t i = 0;
        i < T1_tex_arrays_size;
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
            new_texturearray_i = (int)i;
            new_texture_i = (int32_t)T1_tex_arrays[i].images_size;
            T1_tex_arrays[i].images_size++;
            
            T1_tex_arrays[i].images[new_texture_i].image.width =
                width;
            T1_tex_arrays[i].images[new_texture_i].image.height =
                height;
            T1_tex_arrays[i].images[new_texture_i].image.rgba_values_size =
                width * height * 4;
            T1_tex_arrays[i].images[new_texture_i].image.good = 1;
            
            T1_log_assert(new_texture_i >= 1);
            T1_log_assert(new_texture_i < T1_TEX_SLICES_CAP);
            break;
        }
    }
    
    // if this is a new texturearray (we never saw
    // these image dimensions before)
    if (new_texturearray_i < 0) {
        new_texturearray_i = 0;
        while (
            new_texturearray_i <
                (int32_t)T1_tex_arrays_size &&
            T1_tex_arrays[new_texturearray_i].images_size > 0)
        {
            new_texturearray_i += 1;
        }
        
        if (new_texturearray_i == (int32_t)T1_tex_arrays_size)
        {
            T1_tex_arrays_size++;
        }
        
        T1_log_assert(T1_tex_arrays_size <= T1_TEXARRAYS_CAP);
        new_texture_i = 0;
        
        T1_tex_arrays[new_texturearray_i].deleted = 0;
        T1_tex_arrays[new_texturearray_i].images_size = 1;
        T1_tex_arrays[new_texturearray_i].single_img_width = width;
        T1_tex_arrays[new_texturearray_i].single_img_height = height;
        T1_tex_arrays[new_texturearray_i].is_render_target =
            is_render_target > 0;
        T1_tex_arrays[new_texturearray_i].bc1_compressed =
            use_bc1_compression > 0;
        T1_tex_arrays[new_texturearray_i].images[0].image.width =
            width;
        T1_tex_arrays[new_texturearray_i].images[0].image.height =
            height;
        T1_tex_arrays[new_texturearray_i].images[0].image.rgba_values_size = width * height * 4;
        T1_tex_arrays[new_texturearray_i].images[0].image.good = 1;
    }
    
    T1_std_strcpy_cap(
        T1_tex_arrays[new_texturearray_i].images[new_texture_i].name,
        T1_TEX_NAME_CAP,
        filename);
}

T1Tex T1_tex_array_get_filename_loc(
    const char * for_filename)
{
    // T1_log_assert(for_filename != NULL);
    // T1_log_assert(for_filename[0] != '\0');
    
    T1Tex return_value;
    
    return_value.array_i = -1;
    return_value.slice_i = -1;
    
    if (for_filename == NULL || for_filename[0] == '\0') {
        return return_value;
    }
    
    for (int16_t i = 0; i < (int16_t)T1_tex_arrays_size; i++) {
        T1_log_assert(T1_tex_arrays[i].images_size < 2000);
        for (int16_t j = 0; j < (int16_t)T1_tex_arrays[i].images_size; j++) {
            if (
                T1_std_are_equal_strings(
                    T1_tex_arrays[i].images[j].name,
                    for_filename))
            {
                return_value.array_i = i;
                return_value.slice_i = j;
                break;
            }
        }
    }
    
    return return_value;
}

void T1_tex_array_debug_dump_to_writables(
    const int32_t texture_array_i,
    uint32_t * success)
{
    #if T1_TEXTURES_ACTIVE == T1_ACTIVE
    for (int32_t texture_i = 0; texture_i < 100; texture_i++) {
        uint32_t fetched = 0;
        
        uint32_t rgba_cap = 30000000;
        uint8_t * rgba = T1_mem_malloc_managed(rgba_cap);
        uint32_t rgba_size = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        
        T1_platform_gpu_fetch_rgba_at(
            /* const int32_t texture_array_i: */
                texture_array_i,
            /* const int32_t texture_i: */
                texture_i,
            /* uint8_t *rgba_recipient: */
                rgba,
            /* uint32_t * recipient_size: */
                &rgba_size,
            /* uint32_t * recipient_width: */
                &width,
            /* uint32_t * recipient_height: */
                &height,
            /* uint32_t recipient_cap: */
                rgba_cap,
            /* uint32_t *good: */
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
        T1_std_strcat_int_cap(filename, 128, texture_array_i);
        T1_std_strcat_cap(filename, 128, "_");
        T1_std_strcat_int_cap(filename, 128, texture_i);
        T1_std_strcat_cap(filename, 128, ".bmp");
        
        uint32_t write_good = 0;
        T1_platform_write_rgba_to_writables(
            /* const char * local_filename: */
                filename,
            /* uint8_t * rgba: */
                rgba,
            /* const uint32_t rgba_size: */
                rgba_size,
            /* const uint32_t width: */
                width,
            /* const uint32_t height: */
                height,
            /* uint32_t * good: */
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
