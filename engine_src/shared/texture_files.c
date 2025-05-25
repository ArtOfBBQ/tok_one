#include "texture_files.h"

void texture_files_preregister_png_resource(
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
    texture_array_preregister_null_image(
        /* const char * filename: */
            filename,
        /* const uint32_t height: */
            height,
        /* const uint32_t width: */
            width);
    
    free_from_managed(buf.contents);
    *good = 1;
}

void texture_files_decode_png_resource(
    const char * filename,
    uint32_t * good)
{
    *good = 0;
    
    int32_t texture_array_i = -1;
    int32_t texture_i = -1;
    texture_array_get_filename_location(
            filename,
        /* int32_t * texture_array_i_recipient: */
            &texture_array_i,
        /* int32_t *texture_i_recipient: */
            &texture_i);
    
    if (texture_array_i < 0) {
        return;
    }
    
    FileBuffer file_buffer;
    file_buffer.size_without_terminator = platform_get_resource_size(
        filename);
    
    file_buffer.contents =
        (char *)malloc_from_managed(sizeof(char)
            * file_buffer.size_without_terminator + 1);
    
    platform_read_resource_file(
        filename,
        &file_buffer);
    
    log_assert(file_buffer.good);
    
    texture_array_decode_null_png_at(
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
