#include "T1_texture_files.h"


static void malloc_img_from_filename(
    DecodedImage * recipient,
    const char * filename,
    const uint32_t thread_id)
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
                &recipient->good,
            /* const uint32_t thread_id: */
                thread_id);
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
    } else if (
        file_buffer.contents[0] == 'D' &&
        file_buffer.contents[1] == 'D' &&
        file_buffer.contents[2] == 'S' &&
        file_buffer.contents[3] == ' ')
    {
        recipient->good = false;
        recipient->rgba_values_size =
            (uint32_t)file_buffer.size_without_terminator + 1;
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
        
        common_memcpy(
            /* void * dest: */
                recipient->rgba_values_page_aligned,
            /* const void * src: */
                file_buffer.contents,
            /* size_t n_bytes: */
                recipient->rgba_values_size);
        
        recipient->good = true;
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
    
    return;
}

void T1_texture_files_register_new_by_splitting_file(
    const char * filename,
    const uint32_t rows,
    const uint32_t columns)
{
    DecodedImage stack_img;
    DecodedImage * img = &stack_img;
    malloc_img_from_filename(img, filename, /* thread_id: */ 0);
    
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
            width,
        /* const uint32_t is_dds_image: */
            false);
    
    free_from_managed(buf.contents);
    *good = 1;
}

typedef struct {
    char     magic_number_dds[4];
    uint32_t size;          // Size of the header (must be 124)
    uint32_t flags;         // Header flags
    uint32_t height;        // Image height in pixels
    uint32_t width;         // Image width in pixels
    uint32_t pitchOrLinearSize; // Pitch or linear size
    uint32_t depth;         // Depth (for volume textures)
    uint32_t mipMapCount;   // Number of mip levels
    uint32_t reserved1[11]; // Reserved
    // ... other fields (pixel format, caps, etc.)
} DDS_Header;

void T1_texture_files_preregister_dds_resource(
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
    
    if (!buf.good) {
        return;
    }
    
    DDS_Header * header = (DDS_Header *)buf.contents;
    log_assert(header->magic_number_dds[0] == 'D');
    log_assert(header->magic_number_dds[1] == 'D');
    log_assert(header->magic_number_dds[2] == 'S');
    log_assert(header->magic_number_dds[3] == ' ');
    log_assert(header->size == 124);
    
    if (
        header->magic_number_dds[0] != 'D' ||
        header->magic_number_dds[1] != 'D' ||
        header->magic_number_dds[2] != 'S' ||
        header->magic_number_dds[3] != ' ' ||
        header->size != 124)
    {
        *good = 0;
        return;
    }
    
    T1_texture_array_preregister_null_image(
        /* const char * filename: */
            filename,
        /* const uint32_t height: */
            header->height,
        /* const uint32_t width: */
            header->width,
        /* const uint32_t is_dds_image: */
            true);
    
    free_from_managed(buf.contents);
    *good = 1;
}

void T1_texture_files_decode_all_preregistered(
    const uint32_t thread_id,
    const uint32_t using_num_threads)
{
    int32_t texture_arrays_to_init = (int32_t)texture_arrays_size - 1;
    int32_t texture_arrays_per_thread =
        texture_arrays_to_init / (int32_t)using_num_threads;
    
    int32_t start_ta_i = 1 + ((int32_t)thread_id * texture_arrays_per_thread);
    int32_t end_ta_i =
        thread_id + 1 >= using_num_threads ?
            (int32_t)texture_arrays_size :
            start_ta_i + texture_arrays_per_thread;
    
    log_assert(using_num_threads > 0);
    log_assert(using_num_threads < 7);
    
    for (int32_t ta_i = start_ta_i; ta_i < end_ta_i; ta_i++) {
        if (!texture_arrays[ta_i].bc1_compressed) {
            engine_globals->startup_bytes_to_load +=
                texture_arrays[ta_i].single_img_width *
                texture_arrays[ta_i].single_img_height *
                4 *
                texture_arrays[ta_i].images_size;
        }
    }
    
    for (int32_t ta_i = start_ta_i; ta_i < end_ta_i; ta_i++) {
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
            
            malloc_img_from_filename(
                /* DecodedImage * recipient: */
                    &texture_arrays[ta_i].images[t_i].image,
                /* const char * filename: */
                    texture_arrays[ta_i].images[t_i].filename,
                /* const uint32_t thread_id: */
                    thread_id);
            
            if (!texture_arrays[ta_i].bc1_compressed) {
                engine_globals->startup_bytes_loaded += (
                    texture_arrays[ta_i].single_img_height *
                    texture_arrays[ta_i].single_img_width *
                    4);
            }
        }
    }
}
