#include "T1_texture_array.h"


static uint32_t texture_arrays_mutex_ids[TEXTUREARRAYS_SIZE];

T1TextureArray * T1_texture_arrays = NULL;
uint32_t T1_texture_arrays_size = 0;

void T1_texture_array_push_all_predecoded(void)
{
    #if T1_TEXTURES_ACTIVE == T1_ACTIVE
    for (
        int32_t ta_i = 1;
        ta_i < (int32_t)T1_texture_arrays_size;
        ta_i++)
    {
        if (
            T1_texture_arrays[ta_i].images_size < 1 ||
            T1_texture_arrays[ta_i].gpu_capacity > 0)
        {
            continue;
        }
        
        T1_platform_gpu_copy_texture_array(
            /* const int32_t texture_array_i: */
                ta_i,
            /* const uint32_t num_images: */
                T1_texture_arrays[ta_i].images_size,
            /* const uint32_t single_image_width: */
                T1_texture_arrays[ta_i].single_img_width,
            /* const uint32_t single_image_height: */
                T1_texture_arrays[ta_i].single_img_height,
            /* const bool32_t is_render_target: */
                false,
            /* const bool32_t use_bc1_compression: */
                T1_texture_arrays[ta_i].bc1_compressed);
        
        for (
            int32_t t_i = 0;
            t_i < (int32_t)T1_texture_arrays[ta_i].images_size;
            t_i++)
        {
            uint8_t * rgba_values_freeable = NULL;
            uint8_t * rgba_values_page_aligned = NULL;
            uint32_t use_error_image = false;
            
            if (
                !T1_texture_arrays[ta_i].images[t_i].image.good ||
                T1_texture_arrays[ta_i].images[t_i].image.
                    rgba_values_freeable == NULL ||
                T1_texture_arrays[ta_i].images[t_i].image.
                    rgba_values_page_aligned == NULL)
            {
                use_error_image = true;
                
                uint32_t rgba_size =
                    T1_texture_arrays[ta_i].single_img_width *
                        T1_texture_arrays[ta_i].single_img_height * 4;
                T1_mem_malloc_from_managed_page_aligned(
                    /* void ** base_pointer_for_freeing: */
                       (void **)&rgba_values_freeable,
                    /* void ** aligned_subptr: */
                       (void **)&rgba_values_page_aligned,
                    /* const size_t subptr_size: */
                        rgba_size);
                
                uint32_t packed_rgba_red_u32 =
                    ((uint32_t)255 << 24) | // Alpha
                    ((uint32_t)  5 << 16) | // Blue
                    ((uint32_t)  5 <<  8) | // Green
                    255;          // Red
                uint32_t packed_rgba_blue_u32 =
                    ((uint32_t)255 << 24) | // Alpha
                    ((uint32_t)255 << 16) | // Blue
                    ((uint32_t)255 <<  8) | // Green
                    0;            // Red
                
                int32_t packed_rgba_red;
                T1_std_memcpy(&packed_rgba_red, &packed_rgba_red_u32, 4);
                int32_t packed_rgba_blue;
                T1_std_memcpy(&packed_rgba_blue, &packed_rgba_blue_u32, 4);
                
                uint32_t one_third = (rgba_size / 3);
                while (one_third % 4 != 0) {
                    one_third -= 1;
                }
                
                T1_std_memset_i32(
                    rgba_values_page_aligned,
                    packed_rgba_red,
                    one_third);
                T1_std_memset_i32(
                    rgba_values_page_aligned + (one_third),
                    packed_rgba_blue,
                    one_third);
                T1_std_memset_i32(
                    rgba_values_page_aligned + (one_third * 2),
                    packed_rgba_red,
                    rgba_size - (one_third * 2));
            } else {
                rgba_values_freeable = T1_texture_arrays[ta_i].
                        images[t_i].image.
                            rgba_values_freeable;
                rgba_values_page_aligned = T1_texture_arrays[ta_i].
                        images[t_i].image.
                            rgba_values_page_aligned;
            }
            
            if (T1_texture_arrays[ta_i].bc1_compressed)
            {
                if (!use_error_image) {
                    T1_platform_gpu_push_bc1_texture_slice_and_free_bc1_values(
                        /* const int32_t texture_array_i: */
                            ta_i,
                        /* const int32_t texture_i: */
                            t_i,
                        /* const uint32_t parent_texture_array_images_size: */
                            T1_texture_arrays[ta_i].images_size,
                        /* const uint32_t image_width: */
                            T1_texture_arrays[ta_i].single_img_width,
                        /* const uint32_t image_height: */
                            T1_texture_arrays[ta_i].single_img_height,
                        /* uint8_t * raw_bc1_file_freeable: */
                            rgba_values_freeable,
                        /* uint8_t * raw_bc1_file_page_aligned: */
                            rgba_values_page_aligned);    
                } else {
                    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
                    char errmsg[128];
                    T1_std_strcpy_cap(
                        errmsg,
                        128,
                        "Missing critical bc1 texture: ");
                    T1_std_strcat_cap(
                        errmsg,
                        128,
                        T1_texture_arrays[ta_i].images[t_i].name);
                    T1_std_strcat_cap(
                        errmsg,
                        128,
                        ".\n");
                    log_dump_and_crash("Missing critical bc1 texture\n");
                    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
                    // Pass
                    #else
                    #error
                    #endif
                }
            } else {
                T1_platform_gpu_push_texture_slice_and_free_rgba_values(
                    /* const int32_t texture_array_i: */
                        ta_i,
                    /* const int32_t texture_i: */
                        t_i,
                    /* const uint32_t parent_texture_array_images_size: */
                        T1_texture_arrays[ta_i].images_size,
                    /* const uint32_t image_width: */
                        T1_texture_arrays[ta_i].single_img_width,
                    /* const uint32_t image_height: */
                        T1_texture_arrays[ta_i].single_img_height,
                    /* uint8_t * rgba_values_freeable: */
                        rgba_values_freeable,
                    /* uint8_t * rgba_values_page_aligned: */
                        rgba_values_page_aligned);
            }
        }
    }
    #elif T1_TEXTURES_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
}

static T1DecodedImage * extract_image(
    const T1DecodedImage * original,
    const uint32_t sprite_columns,
    const uint32_t sprite_rows,
    const uint32_t x,
    const uint32_t y)
{
    log_assert(x > 0);
    log_assert(y > 0);
    log_assert(original != NULL);
    log_assert(original->rgba_values_freeable != NULL);
    log_assert(original->rgba_values_page_aligned != NULL);
    
    if (!T1_app_running) { return NULL; }
    log_assert(sprite_columns > 0);
    log_assert(sprite_rows > 0);
    log_assert(x <= sprite_columns);
    log_assert(y <= sprite_rows);
    if (!T1_app_running) { return NULL; }
    
    T1DecodedImage * new_image = T1_mem_malloc_from_unmanaged(sizeof(T1DecodedImage));
    log_assert(new_image != NULL);
    
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    if (!T1_app_running) { return NULL; }
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
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
    log_assert(slice_size_bytes > 0);
    log_assert(slice_width_pixels > 0);
    log_assert(slice_height_pixels > 0);
    log_assert(
        slice_size_bytes == slice_width_pixels * slice_height_pixels * 4);
    log_assert(
        slice_size_bytes *
            sprite_columns *
            sprite_rows ==
                original->rgba_values_size);
    
    new_image->rgba_values_size = slice_size_bytes;
    T1_mem_malloc_from_managed_page_aligned(
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
        
        if (!T1_app_running) {
            new_image->good = false;
            break;
        }
        
        for (
            uint32_t _ = 0;
            _ < (slice_width_pixels * 4);
            _++)
        {
            log_assert((rgba_value_i + _) < original->rgba_values_size);
            log_assert(i < new_image->rgba_values_size);
            new_image->rgba_values_page_aligned[i] =
                original->rgba_values_page_aligned[rgba_value_i + _];
            i++;
        }
    }
    
    new_image->good = true;
    return new_image;
}

void T1_texture_array_init(void) {
    
    // initialize texture arrays
    T1_texture_arrays = (T1TextureArray *)
        T1_mem_malloc_from_unmanaged(
            sizeof(T1TextureArray) *
                TEXTUREARRAYS_SIZE);
    
    T1_std_memset(
        T1_texture_arrays,
        0,
        sizeof(T1TextureArray) * TEXTUREARRAYS_SIZE);
    
    for (uint32_t i = 0; i < TEXTUREARRAYS_SIZE; i++)
    {
        texture_arrays_mutex_ids[i] = T1_platform_init_mutex_and_return_id();
    }
}

static void
    register_to_texturearray_by_splitting_image(
        T1DecodedImage * new_image,
        const char * filename_prefix,
        const int32_t ta_i,
        const uint32_t rows,
        const uint32_t columns)
{
    log_assert(new_image != NULL);
    if (new_image == NULL) { return; }
    log_assert(new_image->rgba_values_freeable != NULL);
    log_assert(new_image->rgba_values_size > 0);
    log_assert(rows >= 1);
    log_assert(columns >= 1);
    
    if (new_image->rgba_values_size < 1) { return; }
        
    char * filenames[256];
    for (uint32_t i = 0; i < 256; i++) {
        filenames[i] = T1_mem_malloc_from_managed(256);
        T1_std_memset(filenames[i], 0, sizeof(rows * columns * 256));
    }
    
    // each image should have the exact same dimensions
    uint32_t expected_width = new_image->width / columns;
    uint32_t expected_height = new_image->height / rows;
    T1_texture_arrays[ta_i].single_img_width  = expected_width;
    T1_texture_arrays[ta_i].single_img_height = expected_height;
    
    T1_platform_gpu_copy_texture_array(
        /* const int32_t texture_array_i: */
            ta_i,
        /* const uint32_t num_images: */
            rows * columns,
        /* const uint32_t single_image_width: */
            T1_texture_arrays[ta_i].single_img_width,
        /* const uint32_t single_image_height: */
            T1_texture_arrays[ta_i].single_img_height,
        /* const bool32_t is_render_target: */
            false,
        /* const uint32_t use_bc1_compression: */
            false);
    
    for (int32_t row_i = 0; row_i < (int32_t)rows; row_i++) {
        for (int32_t col_i = 0; col_i < (int32_t)columns; col_i++) {
            
            int32_t t_i = (row_i * ((int32_t)columns)) + col_i;
            
            T1DecodedImage * split_img = extract_image(
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
                log_assert(0);
                continue;
            }
            log_assert(split_img->good);
            
            log_assert(split_img->width  == expected_width);
            log_assert(split_img->height == expected_height);
            
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
                T1_texture_arrays[ta_i].images[t_i].name,
                FILENAME_MAX,
                filenames[(row_i*(int32_t)columns)+col_i]);
            T1_texture_arrays[ta_i].images_size += 1;
            
            T1_platform_gpu_push_texture_slice_and_free_rgba_values(
                /* const int32_t texture_array_i: */
                    ta_i,
                /* const int32_t texture_i: */
                    t_i,
                /* const uint32_t parent_texture_array_images_size: */
                    T1_texture_arrays[ta_i].images_size,
                /* const uint32_t image_width: */
                    T1_texture_arrays[ta_i].single_img_width,
                /* const uint32_t image_height: */
                    T1_texture_arrays[ta_i].single_img_height,
                /* uint8_t * rgba_values_freeable: */
                    split_img->rgba_values_freeable,
                /* uint8_t * rgba_values_page_aligned: */
                    split_img->rgba_values_page_aligned);
        }
    }
    
    for (uint32_t i = 0; i < 256; i++) {
        free(filenames[i]);
    }
}

int32_t T1_texture_array_create_new_render_view(
    const uint32_t width,
    const uint32_t height)
{
    log_assert(T1_render_views != NULL);
    
    int32_t rv_i = T1_render_view_fetch_next(
        width,
        height);
    if (rv_i < 0) { return rv_i; }
    
    T1_render_views->cpu[rv_i].write_type =
        T1RENDERVIEW_WRITE_RENDER_TARGET;
    log_assert(T1_render_views->cpu[rv_i].width ==
        width);
    log_assert(T1_render_views->cpu[rv_i].height ==
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
        T1_texture_array_get_filename_location(
            tex_name);
    
    log_assert(existing.array_i < 0);
    log_assert(existing.slice_i < 0);
    
    T1_texture_array_postregister_null_image(
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
        T1_texture_array_get_filename_location(
            tex_name);
    
    T1_render_views->cpu[rv_i].write_array_i =
        tex.array_i;
    T1_render_views->cpu[rv_i].write_slice_i =
        tex.slice_i;
    log_assert(
        T1_render_views->cpu[rv_i].write_array_i >= 1);
    log_assert(
        T1_render_views->cpu[rv_i].write_slice_i >= 0);
    
    T1_platform_gpu_copy_texture_array(
        /* const int32_t texture_array_i: */
            tex.array_i,
        /* const uint32_t num_images: */
            T1_texture_arrays[tex.array_i].
                images_size,
        /* const uint32_t single_image_width: */
            T1_texture_arrays[tex.array_i].
                single_img_width,
        /* const uint32_t single_image_height: */
            T1_texture_arrays[tex.array_i].
                single_img_height,
        /* const bool32_t is_render_target: */
            true,
        /* const bool32_t use_bc1_compression: */
            false);
    
    return rv_i;
}

void T1_texture_array_delete_array(
    const int32_t array_i)
{
    log_assert(array_i != 0);
    
    T1_platform_gpu_delete_texture_array(array_i);
    
    T1_std_memset(
        T1_texture_arrays + array_i,
        0,
        sizeof(T1TextureArray));
    T1_texture_arrays[array_i].deleted = true;
}

void T1_texture_array_delete_slice(
    const int32_t array_i,
    const int32_t slice_i)
{
    log_assert(array_i >= 0);
    log_assert(slice_i >= 0);
    if (array_i != DEPTH_TEXTUREARRAYS_I) {
        log_assert(
            slice_i < (int32_t)
                T1_texture_arrays[array_i].images_size);
        log_assert(
            !T1_texture_arrays[array_i].images[slice_i].
                deleted);
    }
    
    if (array_i != DEPTH_TEXTUREARRAYS_I) {
        log_assert(
            array_i < (int32_t)T1_texture_arrays_size);
        
        T1_std_memset(
            T1_texture_arrays[array_i].images + slice_i,
            0,
            sizeof(T1TextureArrayImage));
        T1_texture_arrays[array_i].images[slice_i].
            deleted = true;
        
        while (
            T1_texture_arrays[array_i].images_size > 0 &&
            T1_texture_arrays[array_i].images[
                T1_texture_arrays[array_i].images_size - 1].
                    deleted)
        {
            T1_texture_arrays[array_i].images_size -= 1;
        }
        
        if (
            T1_texture_arrays[array_i].images_size == 0)
        {
            T1_texture_array_delete_array(array_i);
        }
    } else {
        T1_platform_gpu_delete_depth_tex(slice_i);
    }
}

void T1_texture_array_register_new_by_splitting_image(
    T1DecodedImage * new_image,
    const char * filename_prefix,
    const uint32_t rows,
    const uint32_t columns)
{
    log_assert(new_image != NULL);
    if (new_image == NULL) { return; }
    
    int32_t new_texture_array_i = (int)T1_texture_arrays_size;
    T1_texture_arrays[new_texture_array_i].request_init = true;
    T1_texture_arrays_size++;
    log_assert(T1_texture_arrays_size <= TEXTUREARRAYS_SIZE);
    
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

void T1_texture_array_preregister_null_image(
    const char * filename,
    const uint32_t width,
    const uint32_t height,
    const uint32_t is_render_target,
    const uint32_t use_bc1_compression)
{
    int32_t new_texturearray_i = -1;
    int32_t new_texture_i = -1;
    
    // *****************
    // find out if same width/height was already used
    // set new_texturearray_i and new_texture_i if so
    for (
        uint32_t i = 0;
        i < T1_texture_arrays_size;
        i++)
    {
        log_assert(i < TEXTUREARRAYS_SIZE);
        if (
            T1_texture_arrays[i].images_size > 0 &&
            T1_texture_arrays[i].
                single_img_width  == width &&
            T1_texture_arrays[i].
                single_img_height == height &&
            T1_texture_arrays[i].
                is_render_target == is_render_target &&
            T1_texture_arrays[i].bc1_compressed ==
                use_bc1_compression)
        {
            new_texturearray_i = (int)i;
            new_texture_i = (int32_t)T1_texture_arrays[i].images_size;
            T1_texture_arrays[i].images_size++;
            
            log_assert(new_texture_i >= 1);
            log_assert(new_texture_i < MAX_FILES_IN_SINGLE_TEXARRAY);
            break;
        }
    }
    
    // if this is a new texturearray (we never saw
    // these image dimensions before)
    if (new_texturearray_i < 0) {
        new_texturearray_i = 0;
        while (
            new_texturearray_i <
                (int32_t)T1_texture_arrays_size &&
            T1_texture_arrays[new_texturearray_i].images_size > 0)
        {
            new_texturearray_i += 1;
        }
        
        if (new_texturearray_i == (int32_t)T1_texture_arrays_size)
        {
            T1_texture_arrays_size++;
        }
        
        log_assert(
            T1_texture_arrays_size <=
                TEXTUREARRAYS_SIZE);
        new_texture_i = 0;
        
        T1_texture_arrays[new_texturearray_i].
            images_size = 1;
        T1_texture_arrays[new_texturearray_i].
            single_img_width = width;
        T1_texture_arrays[new_texturearray_i].
            single_img_height = height;
        T1_texture_arrays[new_texturearray_i].
            is_render_target = is_render_target;
        T1_texture_arrays[new_texturearray_i].
            bc1_compressed =
                use_bc1_compression;
    }
    
    T1_std_strcpy_cap(
        T1_texture_arrays[new_texturearray_i].images[new_texture_i].name,
        TEXTUREARRAY_FILENAME_SIZE,
        filename);
}

void T1_texture_array_postregister_null_image(
    const char * filename,
    const uint32_t width,
    const uint32_t height,
    const uint32_t is_render_target,
    const uint32_t use_bc1_compression)
{
    if (
        !is_render_target &&
        T1_std_string_starts_with(filename, "__%T1%__"))
    {
        log_dump_and_crash(
            "The __%T1%__ prefix"
            " is reserved for internal textures.");
        return;
    }
    
    int32_t new_array_i = -1;
    int32_t new_slice_i = -1;
    
    // *****************
    // find out if same width/height was already used
    // set new_texturearray_i and new_texture_i if so
    for (
        uint32_t i = 0;
        i < T1_texture_arrays_size;
        i++)
    {
        log_assert(i < TEXTUREARRAYS_SIZE);
        if (
            T1_texture_arrays[i].
                single_img_width == width &&
            T1_texture_arrays[i].
                single_img_height == height &&
            T1_texture_arrays[i].
                is_render_target == is_render_target &&
            T1_texture_arrays[i].
                bc1_compressed == use_bc1_compression)
        {
            new_array_i = (int)i;
            new_slice_i = 0;
            while (
                !T1_texture_arrays[new_array_i].
                    images[new_slice_i].deleted &&
                new_slice_i < (int32_t)
                    T1_texture_arrays[new_array_i].
                        images_size)
            {
                new_slice_i += 1;
            }
            
            if (
                new_slice_i == (int32_t)
                    T1_texture_arrays[new_array_i].
                        images_size)
            {
                T1_texture_arrays[new_array_i].
                    images_size += 1;
            }
            
            log_assert(new_slice_i >= 0);
            log_assert(new_slice_i < (int32_t)
                T1_texture_arrays[new_array_i].
                    images_size);
            log_assert(new_slice_i <
                MAX_FILES_IN_SINGLE_TEXARRAY);
            break;
        }
    }
    
    // if this is a new texturearray (we never saw
    // these image dimensions before)
    if (new_array_i < 0)
    {
        new_array_i = 0;
        while (
            new_array_i <
                (int32_t)T1_texture_arrays_size &&
            T1_texture_arrays[new_array_i].
                images_size > 0)
        {
            new_array_i += 1;
        }
        
        if (new_array_i == (int32_t)
            T1_texture_arrays_size)
        {
            T1_texture_arrays_size++;
        }
        
        log_assert(T1_texture_arrays_size <=
            TEXTUREARRAYS_SIZE);
        new_slice_i = 0;
        
        T1_texture_arrays[new_array_i].
            images_size = 1;
        T1_texture_arrays[new_array_i].
            single_img_width = width;
        T1_texture_arrays[new_array_i].
            single_img_height = height;
        T1_texture_arrays[new_array_i].
            is_render_target = is_render_target;
        T1_texture_arrays[new_array_i].
            bc1_compressed =
                use_bc1_compression;
    }
    
    T1_std_strcpy_cap(
        T1_texture_arrays[new_array_i].
            images[new_slice_i].name,
        TEXTUREARRAY_FILENAME_SIZE,
        filename);
    T1_texture_arrays[new_array_i].
        images[new_slice_i].deleted = false;
}

T1Tex T1_texture_array_get_filename_location(
    const char * for_filename)
{
    // log_assert(for_filename != NULL);
    // log_assert(for_filename[0] != '\0');
    
    T1Tex return_value;
    
    return_value.array_i = -1;
    return_value.slice_i = -1;
    
    if (for_filename == NULL || for_filename[0] == '\0') {
        return return_value;
    }
    
    for (int16_t i = 0; i < (int16_t)T1_texture_arrays_size; i++) {
        log_assert(T1_texture_arrays[i].images_size < 2000);
        for (int16_t j = 0; j < (int16_t)T1_texture_arrays[i].images_size; j++) {
            if (
                T1_std_are_equal_strings(
                    T1_texture_arrays[i].images[j].name,
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

void T1_texture_array_debug_dump_texturearray_to_writables(
    const int32_t texture_array_i,
    uint32_t * success)
{
    #if T1_TEXTURES_ACTIVE == T1_ACTIVE
    for (int32_t texture_i = 0; texture_i < 100; texture_i++) {
        uint32_t fetched = 0;
        
        uint32_t rgba_cap = 30000000;
        uint8_t * rgba = T1_mem_malloc_from_managed(rgba_cap);
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
        
        T1_mem_free_from_managed(rgba);
        
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
