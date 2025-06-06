#include "T1_texture_files.h"

static void malloc_img_from_filename(
    DecodedImage * recipient,
    const char * filename)
{
    FileBuffer file_buffer;
    file_buffer.size_without_terminator = platform_get_resource_size(filename);
    
    if (file_buffer.size_without_terminator < 1) {
        return;
    }
    
    file_buffer.contents =
        (char *)malloc_from_managed(sizeof(char) *
            file_buffer.size_without_terminator + 1);
    common_memset_char(
        file_buffer.contents,
        0,
        file_buffer.size_without_terminator + 1);
    
    platform_read_resource_file(
        filename,
        &file_buffer);
    
    if (!file_buffer.good) {
        free_from_managed((uint8_t *)file_buffer.contents);
        return;
    }
    
    if (
        file_buffer.contents[1] == 'P' &&
        file_buffer.contents[2] == 'N' &&
        file_buffer.contents[3] == 'G')
    {
        get_PNG_width_height(
            /* const uint8_t * compressed_input: */
                (uint8_t *)file_buffer.contents,
            /* const uint64_t compressed_input_size: */
                file_buffer.size_without_terminator - 1,
            /* uint32_t * out_width: */
                &recipient->width,
            /* uint32_t * out_height: */
                &recipient->height,
            /* uint32_t * out_good: */
                &recipient->good);
        
        if (!recipient->good) {
            log_assert(0);
            free_from_managed((uint8_t *)file_buffer.contents);
            return;
        }
        
        recipient->good = false;
        recipient->pixel_count = recipient->width * recipient->height;
        recipient->rgba_values_size = recipient->pixel_count * 4;
        malloc_from_managed_page_aligned(
            /* void *base_pointer_for_freeing: */
                (void *)&recipient->rgba_values_freeable,
            /* void *aligned_subptr: */
                (void *)&recipient->rgba_values_page_aligned,
            /* const size_t subptr_size: */
                recipient->rgba_values_size);
        
        common_memset_char(
            recipient->rgba_values_page_aligned,
            0,
            recipient->rgba_values_size);
        
        decode_PNG(
            /* const uint8_t * compressed_input: */
                (uint8_t *)file_buffer.contents,
            /* const uint64_t compressed_input_size: */
                file_buffer.size_without_terminator - 1,
            /* out_rgba_values: */
                recipient->rgba_values_page_aligned,
            /* rgba_values_size: */
                recipient->rgba_values_size,
            /* uint32_t * out_good: */
                &recipient->good);
    } else if (
        file_buffer.contents[0] == 'B' &&
        file_buffer.contents[1] == 'M')
    {
        get_BMP_width_height(
            /* const uint8_t * compressed_input: */
                (uint8_t *)file_buffer.contents,
            /* const uint64_t compressed_input_size: */
                file_buffer.size_without_terminator - 1,
            /* uint32_t * out_width: */
                &recipient->width,
            /* uint32_t * out_height: */
                &recipient->height,
            /* uint32_t * out_good: */
                &recipient->good);
        
        if (!recipient->good) {
            log_assert(0);
            free_from_managed((uint8_t *)file_buffer.contents);
            return;
        }
        
        recipient->good = false;
        recipient->pixel_count = recipient->width * recipient->height;
        recipient->rgba_values_size = recipient->pixel_count * 4;
        malloc_from_managed_page_aligned(
            /* void * base_pointer_for_freeing: */
                (void *)&recipient->rgba_values_freeable,
            /* void * aligned_subptr: */
                (void *)&recipient->rgba_values_page_aligned,
            /* const size_t subptr_size: */
                recipient->rgba_values_size);
        common_memset_char(
            recipient->rgba_values_page_aligned,
            0,
            recipient->rgba_values_size);
        
        decode_BMP(
            /* raw_input: */ (uint8_t *)file_buffer.contents,
            /* raw_input_size: */ file_buffer.size_without_terminator - 1,
            /* out_rgba_values: */ recipient->rgba_values_page_aligned,
            /* out_rgba_values_size: */ recipient->rgba_values_size,
            /* out_good: */ &recipient->good);
    } else {
        log_append("unrecognized file format in: ");
        log_append(filename);
        log_append_char('\n');
        recipient->good = false;
    }
    free_from_managed(file_buffer.contents);
    
    if (!recipient->good) {
        return;
    }
    
    log_assert(recipient->pixel_count * 4 == recipient->rgba_values_size);
    log_assert(recipient->pixel_count == recipient->width * recipient->height);
        
    log_assert(recipient->good);
    return;
}

void T1_texture_files_register_new_by_splitting_file(
    const char * filename,
    const uint32_t rows,
    const uint32_t columns)
{
    DecodedImage stack_img;
    DecodedImage * img = &stack_img;
    malloc_img_from_filename(img, filename);
    
    log_assert(img->good);
    if (!img->good) { return; }
    
    char filename_prefix[256];
    common_strcpy_capped(filename_prefix, 256, filename);
    uint32_t i = 0;
    while (filename_prefix[i] != '\0' && filename_prefix[i] != '.') {
        i++;
    }
    filename_prefix[i] = '\0';
    
    T1_texture_array_register_new_by_splitting_image(
        img,
        filename_prefix,
        rows,
        columns);
}

void T1_texture_files_preregister_png_resource(
    const char * filename,
    uint32_t * good)
{
    *good = 0;
    
    FileBuffer buf;
    buf.size_without_terminator = platform_get_resource_size(filename);
    if (buf.size_without_terminator > 28) {
        buf.size_without_terminator = 28;
    } else {
        return;
    }
    buf.contents = malloc_from_managed(buf.size_without_terminator+1);
    buf.good = 0;
    
    platform_read_resource_file(filename, &buf);
    uint32_t width = 0;
    uint32_t height = 0;
    get_PNG_width_height(
        /* const uint8_t *compressed_input: */
            (uint8_t *)buf.contents,
        /* const uint64_t compressed_input_size: */
            buf.size_without_terminator,
        /* uint32_t *out_width: */
            &width,
        /* uint32_t *out_height: */
            &height,
        /* uint32_t *out_good: */
            &buf.good);
    if (!buf.good) {
        free_from_managed(buf.contents);
        return;
    }
    T1_texture_array_preregister_null_image(
        /* const char * filename: */
            filename,
        /* const uint32_t height: */
            height,
        /* const uint32_t width: */
            width);
    
    free_from_managed(buf.contents);
    *good = 1;
}

void T1_texture_files_push_all_preregistered(void) {
    
    for (uint32_t ta_i = 1; ta_i < texture_arrays_size; ta_i++) {
        log_assert(texture_arrays[ta_i].images_size > 0);
        log_assert(texture_arrays[ta_i].images_size < MAX_FILES_IN_SINGLE_TEXARRAY);
        log_assert(texture_arrays[ta_i].single_img_width > 0);
        log_assert(texture_arrays[ta_i].single_img_height > 0);
        
        for (
            uint32_t t_i = 0;
            t_i < texture_arrays[ta_i].images_size;
            t_i++)
        {
            if (texture_arrays[ta_i].images[t_i].filename[0] == '\0') {
                continue;
            }
            
            log_assert(!texture_arrays[ta_i].images[t_i].image.good);
            log_assert(texture_arrays[ta_i].images[t_i].image.rgba_values_freeable == NULL);
            log_assert(texture_arrays[ta_i].images[t_i].image.rgba_values_page_aligned == NULL);
            log_assert(texture_arrays[ta_i].images[t_i].image.rgba_values_size == 0);
            
            // texture_arrays[ta_i].images[t_i].image
            malloc_img_from_filename(
                /* DecodedImage * recipient: */
                    &texture_arrays[ta_i].images[t_i].image,
                /* const char * filename: */
                    texture_arrays[ta_i].images[t_i].filename);
        }
    }
    
    T1_texture_array_push_all_preregistered();
}

void T1_texture_files_decode_png_resource(
    const char * filename,
    uint32_t * good)
{
    *good = 0;
    
    int32_t texture_array_i = -1;
    int32_t texture_i = -1;
    T1_texture_array_get_filename_location(
            filename,
        /* int32_t * texture_array_i_recipient: */
            &texture_array_i,
        /* int32_t *texture_i_recipient: */
            &texture_i);
    
    if (texture_array_i < 0) {
        return;
    }
    
    FileBuffer file_buffer;
    file_buffer.good = 0;
    file_buffer.size_without_terminator = platform_get_resource_size(
        filename);
    
    file_buffer.contents =
        (char *)malloc_from_managed(sizeof(char)
            * file_buffer.size_without_terminator + 1);
    
    platform_read_resource_file(
        filename,
        &file_buffer);
    
    log_assert(file_buffer.good);
    
    T1_texture_array_decode_null_png_at(
        /* const uint8_t * rgba_values: */
            (uint8_t *)file_buffer.contents,
        /* const uint32_t rgba_values_size: */
            (uint32_t)file_buffer.size_without_terminator,
        /* const int32_t texture_array_i: */
            texture_array_i,
        /* const int32_t texture_i: */
            texture_i);
    
    *good = 1;
}
