#include "T1_tex_files.h"

#include "decode_png.h"
#include "decode_bmp.h"
#include "T1_global.h"
#include "T1_mem.h"
#include "T1_log.h"
#include "T1_img.h"
#include "T1_tex_array.h"
#include "T1_platform_layer.h"

static void malloc_img_from_resource_name(
    T1Img * recipient,
    const char * filename,
    const uint32_t thread_id)
{
    uint64_t size_without_terminator =
        T1_os_get_resource_size(filename);
    char * contents = NULL;
    bool8_t good = 0;
    
    if (size_without_terminator < 1) {
        return;
    }
    
    contents = (char *)T1_mem_malloc_managed(sizeof(char) *
        size_without_terminator + 1);
    
    T1_std_memset(
        contents,
        0,
        size_without_terminator + 1);
    
    T1_os_read_resource_file(
        filename,
        contents,
        size_without_terminator,
        &good);
    
    if (!good) {
        T1_mem_free_managed((uint8_t *)contents);
        return;
    }
    
    if (
        contents[1] == 'P' &&
        contents[2] == 'N' &&
        contents[3] == 'G')
    {
        decode_png_get_width_height(
            /* const uint8_t * compressed_input: */
                (uint8_t *)contents,
            /* const uint64_t compressed_input_size: */
                size_without_terminator - 1,
            /* uint32_t * out_width: */
                &recipient->width,
            /* uint32_t * out_height: */
                &recipient->height,
            /* uint32_t * out_good: */
                &recipient->good);
        
        if (!recipient->good) {
            T1_log_assert(0);
            T1_mem_free_managed((uint8_t *)contents);
            return;
        }
        
        recipient->good = false;
        recipient->pixel_count = recipient->width * recipient->height;
        recipient->rgba_values_size =
            recipient->pixel_count * 4;
        T1_mem_malloc_managed_page_aligned(
            /* void *base_pointer_for_freeing: */
                (void *)&recipient->rgba_values_freeable,
            /* void *aligned_subptr: */
                (void *)&recipient->rgba_values_page_aligned,
            /* const size_t subptr_size: */
                recipient->rgba_values_size);
        
        T1_std_memset(
            recipient->rgba_values_page_aligned,
            0,
            recipient->rgba_values_size);
        
        decode_png(
            /* const uint8_t * compressed_input: */
                (uint8_t *)contents,
            /* const uint64_t compressed_input_size: */
                size_without_terminator - 1,
            /* out_rgba_values: */
                recipient->rgba_values_page_aligned,
            /* rgba_values_size: */
                recipient->rgba_values_size,
            /* const uint32_t thread_id: */
                thread_id,
            /* uint8_t * out_good: */
                &recipient->good);
    } else if (
        contents[0] == 'B' &&
        contents[1] == 'M')
    {
        get_BMP_width_height(
            /* const uint8_t * compressed_input: */
                (uint8_t *)contents,
            /* const uint64_t compressed_input_size: */
                size_without_terminator - 1,
            /* uint32_t * out_width: */
                &recipient->width,
            /* uint32_t * out_height: */
                &recipient->height,
            /* uint32_t * out_good: */
                &recipient->good);
        
        if (!recipient->good) {
            T1_log_assert(0);
            T1_mem_free_managed((uint8_t *)contents);
            return;
        }
        
        recipient->good = false;
        recipient->pixel_count = recipient->width * recipient->height;
        recipient->rgba_values_size = recipient->pixel_count * 4;
        T1_mem_malloc_managed_page_aligned(
            /* void * base_pointer_for_freeing: */
                (void *)&recipient->rgba_values_freeable,
            /* void * aligned_subptr: */
                (void *)&recipient->rgba_values_page_aligned,
            /* const size_t subptr_size: */
                recipient->rgba_values_size);
        T1_std_memset(
            recipient->rgba_values_page_aligned,
            0,
            recipient->rgba_values_size);
        
        decode_BMP(
            /* raw_input: */ (uint8_t *)contents,
            /* raw_input_size: */ size_without_terminator - 1,
            /* out_rgba_values: */ recipient->rgba_values_page_aligned,
            /* out_rgba_values_size: */ recipient->rgba_values_size,
            /* out_good: */ &recipient->good);
    } else if (
        contents[0] == 'D' &&
        contents[1] == 'D' &&
        contents[2] == 'S' &&
        contents[3] == ' ')
    {
        recipient->good = false;
        T1_log_assert(recipient->width > 0);
        T1_log_assert(recipient->height > 0);
        recipient->pixel_count = recipient->width * recipient->height;
        recipient->rgba_values_size =
            (uint32_t)size_without_terminator + 1;
        T1_mem_malloc_managed_page_aligned(
            /* void *base_pointer_for_freeing: */
                (void *)&recipient->rgba_values_freeable,
            /* void *aligned_subptr: */
                (void *)&recipient->rgba_values_page_aligned,
            /* const size_t subptr_size: */
                recipient->rgba_values_size);
        
        T1_std_memset(
            recipient->rgba_values_page_aligned,
            0,
            recipient->rgba_values_size);
        
        T1_std_memcpy(
            /* void * dest: */
                recipient->rgba_values_page_aligned,
            /* const void * src: */
                contents,
            /* size_t n_bytes: */
                recipient->rgba_values_size);
        
        recipient->good = true;
    } else {
        T1_log_append("unrecognized file format in: ");
        T1_log_append(filename);
        T1_log_append_char('\n');
        recipient->good = false;
    }
    T1_mem_free_managed(contents);
    
    if (!recipient->good) {
        return;
    }
    
    return;
}

void T1_tex_files_reg_new_by_splitting_file(
    const char * filename,
    const uint32_t rows,
    const uint32_t columns)
{
    T1Img stack_img;
    T1Img * img = &stack_img;
    malloc_img_from_resource_name(img, filename, /* thread_id: */ 0);
    
    T1_log_assert(img->good);
    if (!img->good) { return; }
    
    char filename_prefix[256];
    T1_std_strcpy_cap(filename_prefix, 256, filename);
    uint32_t i = 0;
    while (filename_prefix[i] != '\0' && filename_prefix[i] != '.') {
        i++;
    }
    filename_prefix[i] = '\0';
    
    T1_tex_array_reg_new_by_splitting_img(
        img,
        filename_prefix,
        rows,
        columns);
}

void T1_tex_files_load_font_images(
    uint8_t * success,
    char * error_message)
{
    *success = 0;
    
    if (T1_tex_arrays_size != 0) {
        assert(0);
        T1_std_strcpy_cap(
            error_message,
            256,
            "Font error - other textures already "
            "existed");
        return;
    }
    
    const char * fontfile = "font.png";
    T1_tex_files_reg_new_by_splitting_file_error_handling(
        /* filename : */ fontfile,
        /* rows     : */ 10,
        /* columns  : */ 10,
        success,
        error_message);
    T1_tex_arrays[0].request_init = false;
}

void T1_tex_files_reg_new_by_splitting_file_error_handling(
    const char * filename,
    const uint32_t rows,
    const uint32_t columns,
    uint8_t * success,
    char * error_message)
{
    *success = 0;
    
    T1Img stack_img;
    T1Img * img = &stack_img;
    malloc_img_from_resource_name(img, filename, /* thread_id: */ 0);
    
    T1_log_assert(img->good);
    if (!img->good) {
        T1_std_strcpy_cap(
            error_message,
            512,
            "Couldn't read file: ");
        T1_std_strcat_cap(
            error_message,
            512,
            filename);
        return;
    }
    
    char filename_prefix[256];
    T1_std_strcpy_cap(filename_prefix, 256, filename);
    uint32_t i = 0;
    while (filename_prefix[i] != '\0' && filename_prefix[i] != '.') {
        i++;
    }
    filename_prefix[i] = '\0';
    
    T1_tex_array_reg_new_by_splitting_img(
        img,
        filename_prefix,
        rows,
        columns);
    
    *success = 1;
}

#if T1_TEXTURES_ACTIVE == T1_ACTIVE
void T1_tex_files_runtime_reg_png_from_writables(
    const char * filename,
    uint8_t * good)
{
    *good = 0;
    
    char filepath[256];
    T1_std_memset(filepath, 0, 256);
    T1_os_get_writables_dir(filepath, 256);
    T1_os_get_dir_separator(filepath + T1_std_strlen(filepath));
    T1_std_strcat_cap(filepath, 256, filename);
    
    uint64_t contents_cap = T1_os_get_filesize(filepath);
    if (contents_cap <= 28) {
        return;
    }
    char * contents = T1_mem_malloc_managed(contents_cap+1);
    uint32_t contents_size = 0;
    
    T1_os_read_file(
        /* const char * filepath: */
            filepath,
        /* char * recip: */
            contents,
        /* uint32_t * recip_size: */
            &contents_size,
        /* const uint64_t recip_cap: */
            contents_cap,
        /* uint8_t * good: */
            good);
    
    uint32_t width = 0;
    uint32_t height = 0;
    decode_png_get_width_height(
        /* const uint8_t *compressed_input: */
            (uint8_t *)contents,
        /* const uint64_t compressed_input_size: */
            contents_size,
        /* uint32_t *out_width: */
            &width,
        /* uint32_t *out_height: */
            &height,
        /* uint32_t *out_good: */
            good);
    if (!*good) {
        T1_mem_free_managed(contents);
        return;
    }
    
    T1Tex loc = T1_tex_array_reg_img(
        /* const char * filename: */
            filename,
        /* const uint32_t height: */
            height,
        /* const uint32_t width: */
            width,
        /* const uint32_t is_render_target,: */
            false,
        /* const uint32_t is_dds_image: */
            false);
    
    T1Img * recipient =
        &T1_tex_arrays[T1_tex_to_array_i(loc)].images[T1_tex_to_slice_i(loc)].image;
    
    recipient->good = false;
    recipient->width = width;
    recipient->height = height;
    recipient->pixel_count = recipient->width * recipient->height;
    recipient->rgba_values_size = recipient->pixel_count * 4;
    T1_mem_malloc_managed_page_aligned(
        /* void *base_pointer_for_freeing: */
            (void *)&recipient->rgba_values_freeable,
        /* void *aligned_subptr: */
            (void *)&recipient->rgba_values_page_aligned,
        /* const size_t subptr_size: */
            recipient->rgba_values_size);
    
    T1_std_memset(
        recipient->rgba_values_page_aligned,
        0,
        recipient->rgba_values_size);
    
    decode_png(
        /* const uint8_t * compressed_input: */
            (uint8_t *)contents,
        /* const uint64_t compressed_input_size: */
            contents_size,
        /* const uint8_t * out_rgba_values: */
            T1_tex_arrays[T1_tex_to_array_i(loc)].images[T1_tex_to_slice_i(loc)].image.
                rgba_values_freeable,
        /* const uint64_t rgba_values_size: */
            T1_tex_arrays[T1_tex_to_array_i(loc)].images[T1_tex_to_slice_i(loc)].image.
                rgba_values_size,
        /* const uint32_t thread_id: */
            0,
        /* uint8_t * out_good: */
            &T1_tex_arrays[T1_tex_to_array_i(loc)].images[T1_tex_to_slice_i(loc)].image.good);
    
    T1_os_gpu_push_tex_slice_and_free_rgba(
        /* const int32_t texture_array_i: */
            T1_tex_to_array_i(loc),
        /* const int32_t texture_i: */
            T1_tex_to_slice_i(loc));
    
    T1_mem_free_managed(contents);
    *good = 1;
}
#elif T1_TEXTURES_ACTIVE == T1_INACTIVE
#else
#error
#endif

void T1_tex_files_prereg_png_res(
    const char * filename,
    bool8_t * good)
{
    *good = 0;
    
    uint64_t contents_cap = T1_os_get_resource_size(filename);
    if (contents_cap > 28) {
        contents_cap = 28;
    } else {
        return;
    }
    char * contents = T1_mem_malloc_managed(contents_cap+1);
    
    T1_os_read_resource_file(filename, contents, contents_cap, good);
    if (!*good) { return; } else { *good = 0; }
    
    uint32_t width = 0;
    uint32_t height = 0;
    decode_png_get_width_height(
        /* const uint8_t *compressed_input: */
            (uint8_t *)contents,
        /* const uint64_t compressed_input_size: */
            contents_cap,
        /* uint32_t *out_width: */
            &width,
        /* uint32_t *out_height: */
            &height,
        /* uint32_t *out_good: */
            good);
    if (!*good) {
        T1_mem_free_managed(contents);
        return;
    } else { *good = 0; }
    
    T1_tex_array_reg_img(
        /* const char * filename: */
            filename,
        /* const uint32_t width: */
            width,
        /* const uint32_t height: */
            height,
        /* const uint32_t is_render_target: */
            false,
        /* const uint32_t is_dds_image: */
            false);
    
    T1_mem_free_managed(contents);
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

void T1_tex_files_prereg_dds_res(
    const char * filename,
    bool8_t * good)
{
    *good = 0;
    
    uint64_t contents_cap = T1_os_get_resource_size(filename);
    if (contents_cap > 28) {
        contents_cap = 28;
    } else {
        return;
    }
    char * contents = T1_mem_malloc_managed(contents_cap+1);
    
    T1_os_read_resource_file(filename, contents, contents_cap, good);
    
    if (!*good) { return; } else { *good = 0; }
    
    DDS_Header * header = (DDS_Header *)contents;
    T1_log_assert(header->magic_number_dds[0] == 'D');
    T1_log_assert(header->magic_number_dds[1] == 'D');
    T1_log_assert(header->magic_number_dds[2] == 'S');
    T1_log_assert(header->magic_number_dds[3] == ' ');
    T1_log_assert(header->size == 124);
    
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
    
    T1_tex_array_reg_img(
        /* const char * filename: */
            filename,
        /* const uint32_t width: */
            header->width,
        /* const uint32_t height: */
            header->height,
        /* const uint32_t is_render_target: */
            false,
        /* const uint32_t is_dds_image: */
            true);
    
    T1_mem_free_managed(contents);
    *good = 1;
}

void T1_tex_files_decode_all_prereg(
    const uint32_t thread_id,
    const uint32_t using_num_threads)
{
    int32_t texture_arrays_to_init = (int32_t)T1_tex_arrays_size - 1;
    int32_t texture_arrays_per_thread =
        texture_arrays_to_init / (int32_t)using_num_threads;
    if (texture_arrays_per_thread < 1) {
        texture_arrays_per_thread = 1;
    }
    
    int32_t start_ta_i = 1 + ((int32_t)thread_id * texture_arrays_per_thread);
    int32_t end_ta_i =
        thread_id + 1 >= using_num_threads ?
            (int32_t)T1_tex_arrays_size :
            start_ta_i + texture_arrays_per_thread;
    
    T1_log_append("Thread ");
    T1_log_append_uint(thread_id);
    T1_log_append("/");
    T1_log_append_uint(using_num_threads);
    T1_log_append(" decoding texarrays ");
    T1_log_append_int(start_ta_i);
    T1_log_append(" - ");
    T1_log_append_int(end_ta_i);
    T1_log_append("...\n");
    
    T1_log_assert(using_num_threads > 0);
    T1_log_assert(using_num_threads < 7);
    
    for (int32_t ta_i = start_ta_i; ta_i < end_ta_i; ta_i++) {
        T1_tex_arrays[ta_i].started_decoding =
            T1_os_get_current_time_us();
        if (T1_tex_arrays[ta_i].images_size < 1) {
            continue;
        }
        T1_log_assert(T1_tex_arrays[ta_i].images_size <
            T1_TEX_SLICES_CAP);
        T1_log_assert(T1_tex_arrays[ta_i].single_img_width > 0);
        T1_log_assert(T1_tex_arrays[ta_i].single_img_height > 0);
        
        for (
            uint32_t t_i = 0;
            t_i < T1_tex_arrays[ta_i].images_size;
            t_i++)
        {
            if (
                T1_tex_arrays[ta_i].images[t_i].name[0] == '\0' ||
                T1_tex_arrays[ta_i].images[t_i].image.
                    rgba_values_freeable != NULL ||
                T1_tex_arrays[ta_i].images[t_i].image.
                    rgba_values_size == 0 ||
                !T1_tex_arrays[ta_i].images[t_i].image.good)
            {
                continue;
            }
            
            T1_tex_arrays[ta_i].images[t_i].image.good = 0;
            T1_log_assert(T1_tex_arrays[ta_i].images[t_i].
                image.rgba_values_freeable == NULL);
            T1_log_assert(T1_tex_arrays[ta_i].images[t_i].
                image.rgba_values_page_aligned == NULL);
            T1_log_assert(T1_tex_arrays[ta_i].images[t_i].
                image.rgba_values_size > 0);
            
            malloc_img_from_resource_name(
                /* DecodedImage * recipient: */
                    &T1_tex_arrays[ta_i].images[t_i].image,
                /* const char * filename: */
                    T1_tex_arrays[ta_i].images[t_i].name,
                /* const uint32_t thread_id: */
                    thread_id);
            
            if (!T1_tex_arrays[ta_i].bc1_compressed) {
                T1_global->startup_bytes_loaded += (
                    T1_tex_arrays[ta_i].single_img_height *
                    T1_tex_arrays[ta_i].single_img_width *
                    4);
            }
        }
        
        T1_tex_arrays[ta_i].ended_decoding = T1_os_get_current_time_us();
    }
}
