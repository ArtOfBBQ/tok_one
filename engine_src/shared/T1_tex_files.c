#include "T1_tex_files.h"

#include "decode_png.h"
#include "decode_bmp.h"
#include "T1_global.h"
#include "T1_mem.h"
#include "T1_log.h"
#include "T1_img.h"
#include "T1_tex_array.h"
#include "T1_os.h"

static void malloc_img_from_embedded_png(
    T1Img * recip,
    const unsigned char * embedded_data,
    u64 embedded_data_size,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    if (embedded_data_size < 1) {
        *fatal_error = "Embedded data size was 0";
        return;
    }
    
    char * contents = T1_mem_malloc_managed(embedded_data_size);
    if (!contents) {
        *fatal_error = "Malloc fail";
        return;
    }
    T1_std_memcpy(contents, embedded_data, embedded_data_size);
    
    u8 is_png =
        contents[1] == 'P' &&
        contents[2] == 'N' &&
        contents[3] == 'G';
    if (!is_png) {
        *fatal_error = "Error - malloc_img_from_embedded_png(), but embedded data is not a PNG";
        return;
    }
    
    decode_png_get_width_height(
        /* const u8 * compressed_input: */
            (u8 *)embedded_data,
        /* const u64 compressed_input_size: */
            embedded_data_size - 1,
        /* u32 * out_width: */ &recip->width,
        /* u32 * out_height: */ &recip->height,
        /* u32 * out_good: */ fatal_error);
    
    if (*fatal_error != 0) {
        T1_mem_free_managed((u8 *)contents);
        return;
    }
    
    recip->pixel_count = recip->width * recip->height;
    recip->rgba_values_size = recip->pixel_count * 4;
    T1_mem_malloc_managed_page_aligned(
        /* void *base_pointer_for_freeing: */
            (void *)&recip->rgba_values_freeable,
        /* void *aligned_subptr: */
            (void *)&recip->rgba_values_page_aligned,
        /* const u64 subptr_size: */
            recip->rgba_values_size);
    
    T1_std_memset(
        recip->rgba_values_page_aligned,
        0,
        recip->rgba_values_size);
    
    decode_png(
        /* const u8 * compressed_input: */
            (u8 *)contents,
        /* const u64 compressed_input_size: */
            embedded_data_size-1,
        /* out_rgba_values: */
            recip->rgba_values_page_aligned,
        /* rgba_values_size: */
            recip->rgba_values_size,
        /* const u32 thread_id: */
            0,
        /* fatal_error: */
            fatal_error);
    
    T1_mem_free_managed((u8 *)contents);
}

static void malloc_img_from_resource_name(
    T1Img * recipient,
    const char * filename,
    const u32 thread_id,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    if (T1_std_are_equal_strings(filename, "font.png")) {
        malloc_img_from_embedded_png(
            recipient,
            T1_embedded_data_font_png,
            T1_embedded_data_font_png_size,
            fatal_error);
        return;
    }
    
    if (T1_std_are_equal_strings(filename, "perlin_noise.png")) {
        // TODO: use perlin data, not font
        malloc_img_from_embedded_png(
            recipient,
            T1_embedded_data_font_png, // T1_embedded_data_perlin_noise_png,
            T1_embedded_data_font_png_size, // T1_embedded_data_perlin_noise_png_size,
            fatal_error);
        T1_fatal_error_prepend_if_bad(
            fatal_error,
            "Embedded perlin_noise.png: ");
        return;
    }
    
    u64 size_without_terminator =
        T1_os_get_resource_size(filename);
    char * contents = NULL;
    
    if (size_without_terminator < 1) {
        *fatal_error = T1_mem_malloc_unmanaged(512);
        T1_std_memset(*fatal_error, 0, 512);
        T1_std_strcpy_cap(*fatal_error, 512, "Img doesn't exist (filesize 0): ");
        T1_std_strcat_cap(*fatal_error, 512, filename);
        return;
    }
    
    contents = (char *)T1_mem_malloc_managed(sizeof(char) *
        size_without_terminator + 1);
    if (!contents) {
        *fatal_error = "Failed to malloc_managed for img file";
        return;
    }
    
    T1_std_memset(
        contents,
        0,
        size_without_terminator + 1);
    
    T1_os_read_resource_file(
        filename,
        contents,
        size_without_terminator,
        fatal_error);
    
    if (*fatal_error != 0) {
        T1_mem_free_managed((u8 *)contents);
        return;
    }
    
    if (
        contents[1] == 'P' &&
        contents[2] == 'N' &&
        contents[3] == 'G')
    {
        decode_png_get_width_height(
            /* const u8 * compressed_input: */
                (u8 *)contents,
            /* const u64 compressed_input_size: */
                size_without_terminator - 1,
            /* u32 * out_width: */
                &recipient->width,
            /* u32 * out_height: */
                &recipient->height,
            /* fatal_error: */
                fatal_error);
        
        if (*fatal_error != 0) {
            T1_mem_free_managed((u8 *)contents);
            return;
        }
        
        recipient->pixel_count = recipient->width * recipient->height;
        recipient->rgba_values_size =
            recipient->pixel_count * 4;
        T1_mem_malloc_managed_page_aligned(
            /* void *base_pointer_for_freeing: */
                (void *)&recipient->rgba_values_freeable,
            /* void *aligned_subptr: */
                (void *)&recipient->rgba_values_page_aligned,
            /* const u64 subptr_size: */
                recipient->rgba_values_size);
        
        T1_std_memset(
            recipient->rgba_values_page_aligned,
            0,
            recipient->rgba_values_size);
        
        decode_png(
            /* const u8 * compressed_input: */
                (u8 *)contents,
            /* const u64 compressed_input_size: */
                size_without_terminator - 1,
            /* out_rgba_values: */
                recipient->rgba_values_page_aligned,
            /* rgba_values_size: */
                recipient->rgba_values_size,
            /* const u32 thread_id: */
                thread_id,
            /* fatal_error: */
                fatal_error);
    } else if (
        contents[0] == 'B' &&
        contents[1] == 'M')
    {
        get_BMP_width_height(
            /* const u8 * compressed_input: */
                (u8 *)contents,
            /* const u64 compressed_input_size: */
                size_without_terminator - 1,
            /* u32 * out_width: */
                &recipient->width,
            /* u32 * out_height: */
                &recipient->height,
            /* fatal_error: */
                fatal_error);
        
        if (*fatal_error != 0) {
            T1_mem_free_managed((u8 *)contents);
            return;
        }
        
        recipient->pixel_count = recipient->width * recipient->height;
        recipient->rgba_values_size = recipient->pixel_count * 4;
        T1_mem_malloc_managed_page_aligned(
            /* void * base_pointer_for_freeing: */
                (void *)&recipient->rgba_values_freeable,
            /* void * aligned_subptr: */
                (void *)&recipient->rgba_values_page_aligned,
            /* const u64 subptr_size: */
                recipient->rgba_values_size);
        T1_std_memset(
            recipient->rgba_values_page_aligned,
            0,
            recipient->rgba_values_size);
        
        decode_BMP(
            /* raw_input: */ (u8 *)contents,
            /* raw_input_size: */ size_without_terminator - 1,
            /* out_rgba_values: */ recipient->rgba_values_page_aligned,
            /* out_rgba_values_size: */ recipient->rgba_values_size,
            /* out_good: */ fatal_error);
        if (*fatal_error != 0) { return; }
        
    } else if (
        contents[0] == 'D' &&
        contents[1] == 'D' &&
        contents[2] == 'S' &&
        contents[3] == ' ')
    {
        T1_log_assert(recipient->width > 0);
        T1_log_assert(recipient->height > 0);
        recipient->pixel_count = recipient->width * recipient->height;
        recipient->rgba_values_size =
            (u32)size_without_terminator + 1;
        T1_mem_malloc_managed_page_aligned(
            /* void *base_pointer_for_freeing: */
                (void *)&recipient->rgba_values_freeable,
            /* void *aligned_subptr: */
                (void *)&recipient->rgba_values_page_aligned,
            /* const u64 subptr_size: */
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
            /* u64 n_bytes: */
                recipient->rgba_values_size);
    } else {
        *fatal_error = T1_mem_malloc_unmanaged(512);
        T1_std_memset(*fatal_error, 0, 512);
        T1_std_strcpy_cap(*fatal_error, 512, "unrecognized file format in: ");
        T1_std_strcat_cap(*fatal_error, 512, filename);
        return;
    }
    T1_mem_free_managed(contents);
    
    return;
}

void T1_tex_files_reg_new_by_splitting_file(
    const char * filename,
    u32 rows,
    u32 columns,
    b8 free_rgba,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    T1Img stack_img;
    T1Img * img = &stack_img;
    
    malloc_img_from_resource_name(img, filename, /* thread_id: */ 0, fatal_error);
    
    if (*fatal_error != 0) { return; }
    
    char filename_prefix[256];
    T1_std_strcpy_cap(filename_prefix, 256, filename);
    u32 i = 0;
    while (filename_prefix[i] != '\0' && filename_prefix[i] != '.') {
        i++;
    }
    filename_prefix[i] = '\0';
    
    T1_tex_array_reg_new_by_splitting_img(
        img,
        filename_prefix,
        rows,
        columns,
        free_rgba,
        fatal_error);
}

void T1_tex_files_load_font_images(
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    if (T1_tex_arrays_size != 0) {
        *fatal_error = "Font error - other textures already existed";
        return;
    }
    
    const char * fontfile = "font.png";
    T1_tex_files_reg_new_by_splitting_file_error_handling(
        /* filename : */ fontfile,
        /* rows     : */ 10,
        /* columns  : */ 10,
        false,
        fatal_error);
    if (*fatal_error != 0) { return; }
    T1_tex_arrays[0].request_init = false;
}

void T1_tex_files_prereg_and_decode_png_res(
    const char * resource_name,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    T1Img img;
    malloc_img_from_resource_name(&img, resource_name, 0, fatal_error);
    
    if (*fatal_error != 0) { return; }
    
    T1_tex_array_reg_img(
        resource_name,
        img.width,
        img.height,
        false,
        false,
        fatal_error);
    
    if (*fatal_error != 0) { return; }
    
    T1_tex_array_push_all();
}

void T1_tex_files_reg_new_by_splitting_file_error_handling(
    const char * filename,
    u32 rows, u32 columns,
    b8 free_rgba,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    T1Img stack_img;
    T1Img * img = &stack_img;
    malloc_img_from_resource_name(img, filename, /* thread_id: */ 0, fatal_error);
    
    if (*fatal_error != 0) {
        return;
    }
    
    char filename_prefix[256];
    T1_std_strcpy_cap(filename_prefix, 256, filename);
    u32 i = 0;
    while (filename_prefix[i] != '\0' && filename_prefix[i] != '.') {
        i++;
    }
    filename_prefix[i] = '\0';
    
    T1_tex_array_reg_new_by_splitting_img(
        img,
        filename_prefix,
        rows,
        columns,
        free_rgba,
        fatal_error);
}

#if T1_TEXTURES_ACTIVE == T1_ACTIVE
void T1_tex_files_runtime_reg_png_from_writables(
    const char * filename,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    char filepath[256];
    T1_std_memset(filepath, 0, 256);
    T1_os_get_writables_dir(filepath, 256);
    T1_os_get_dir_separator(filepath + T1_std_strlen(filepath));
    T1_std_strcat_cap(filepath, 256, filename);
    
    u64 contents_cap = T1_os_get_filesize(filepath);
    if (*fatal_error != 0) {
        return;
    } else if (contents_cap <= 28) {
        *fatal_error = T1_mem_malloc_unmanaged(512);
        T1_std_memset(*fatal_error, 0, 512);
        T1_std_strcpy_cap(*fatal_error, 512, "Impossibly small filesize for png resource: ");
        T1_std_strcat_cap(*fatal_error, 512, filename);
        T1_std_strcat_cap(*fatal_error, 512, " ");
        T1_std_strcat_u32_cap(*fatal_error, 512, (u32)contents_cap);
        return;
    }
    char * contents = T1_mem_malloc_managed(contents_cap+1);
    u32 contents_size = 0;
    
    T1_os_read_file(
        /* const char * filepath: */
            filepath,
        /* char * recip: */
            contents,
        /* u32 * recip_size: */
            &contents_size,
        /* const u64 recip_cap: */
            contents_cap,
        /* fatal_error: */
            fatal_error);
    
    u32 width = 0;
    u32 height = 0;
    decode_png_get_width_height(
        /* const u8 *compressed_input: */
            (u8 *)contents,
        /* const u64 compressed_input_size: */
            contents_size,
        /* u32 *out_width: */
            &width,
        /* u32 *out_height: */
            &height,
        /* fatal_error: */
            fatal_error);
    if (*fatal_error != 0) {
        T1_mem_free_managed(contents);
        return;
    }
    
    T1Tex loc = T1_tex_array_reg_img(
        /* const char * filename: */
            filename,
        /* const u32 height: */
            height,
        /* const u32 width: */
            width,
        /* const u32 is_render_target,: */
            false,
        /* const u32 is_dds_image: */
            false,
        /* fatal_error: */
            fatal_error);
    
    if (*fatal_error != 0) { return; }
    
    T1Img * recipient =
        &T1_tex_arrays[T1_tex_to_array_i(loc)].images[T1_tex_to_slice_i(loc)].image;
    
    recipient->width = width;
    recipient->height = height;
    recipient->pixel_count = recipient->width * recipient->height;
    recipient->rgba_values_size = recipient->pixel_count * 4;
    T1_mem_malloc_managed_page_aligned(
        /* void *base_pointer_for_freeing: */
            (void *)&recipient->rgba_values_freeable,
        /* void *aligned_subptr: */
            (void *)&recipient->rgba_values_page_aligned,
        /* const u64 subptr_size: */
            recipient->rgba_values_size);
    
    T1_std_memset(
        recipient->rgba_values_page_aligned,
        0,
        recipient->rgba_values_size);
    
    decode_png(
        /* const u8 * compressed_input: */
            (u8 *)contents,
        /* const u64 compressed_input_size: */
            contents_size,
        /* const u8 * out_rgba_values: */
            T1_tex_arrays[T1_tex_to_array_i(loc)].images[T1_tex_to_slice_i(loc)].image.
                rgba_values_freeable,
        /* const u64 rgba_values_size: */
            T1_tex_arrays[T1_tex_to_array_i(loc)].images[T1_tex_to_slice_i(loc)].image.
                rgba_values_size,
        /* const u32 thread_id: */
            0,
        /* fatal_error: */
            fatal_error);
    
    T1_os_gpu_push_tex_slice(
        /* const s32 texture_array_i: */
            T1_tex_to_array_i(loc),
        /* const s32 texture_i: */
            T1_tex_to_slice_i(loc),
        /* free_rgba: */
            true);
    
    T1_mem_free_managed(contents);
}
#elif T1_TEXTURES_ACTIVE == T1_INACTIVE
#else
#error
#endif

void T1_tex_files_prereg_png_res(
    const char * filename,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    T1Tex existing = T1_tex_array_get_filename_loc(filename);
    if (existing != T1_TEX_NONE) {
        T1_fatal_error_new(fatal_error, "T1_tex_files_prereg_png_res() tried to prereg duplicate filename: ");
        T1_std_strcat_cap(*fatal_error, 512, filename);
        return;
    }
    
    u64 contents_cap = T1_os_get_resource_size(filename);
    if (contents_cap > 28) {
        contents_cap = 28;
    } else {
        return;
    }
    char * contents = T1_mem_malloc_managed(contents_cap+1);
    
    T1_os_read_resource_file(filename, contents, contents_cap, fatal_error);
    if (*fatal_error != 0) {
        T1_mem_free_managed(contents);
        return;
    }
    
    u32 width = 0;
    u32 height = 0;
    decode_png_get_width_height(
        /* const u8 *compressed_input: */
            (u8 *)contents,
        /* const u64 compressed_input_size: */
            contents_cap,
        /* u32 *out_width: */
            &width,
        /* u32 *out_height: */
            &height,
        /* fatal_error: */
            fatal_error);
    if (*fatal_error != 0) {
        T1_mem_free_managed(contents);
        return;
    }
    
    T1_tex_array_reg_img(
        /* const char * filename: */
            filename,
        /* const u32 width: */
            width,
        /* const u32 height: */
            height,
        /* const u32 is_render_target: */
            false,
        /* const u32 is_dds_image: */
            false,
        /* fatal_error: */
            fatal_error);
    
    T1_mem_free_managed(contents);
}

typedef struct {
    char     magic_number_dds[4];
    u32 size;          // Size of the header (must be 124)
    u32 flags;         // Header flags
    u32 height;        // Image height in pixels
    u32 width;         // Image width in pixels
    u32 pitchOrLinearSize; // Pitch or linear size
    u32 depth;         // Depth (for volume textures)
    u32 mipMapCount;   // Number of mip levels
    u32 reserved1[11]; // Reserved
    // ... other fields (pixel format, caps, etc.)
} DDS_Header;

void T1_tex_files_prereg_dds_res(
    const char * filename,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    u64 contents_cap = T1_os_get_resource_size(filename);
    if (contents_cap > 28) {
        contents_cap = 28;
    } else {
        return;
    }
    char * contents = T1_mem_malloc_managed(contents_cap+1);
    
    T1_os_read_resource_file(filename, contents, contents_cap, fatal_error);
    
    if (*fatal_error != 0) { return; }
    
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
        *fatal_error = T1_mem_malloc_unmanaged(512);
        T1_std_memset(*fatal_error, 0, 512);
        T1_std_strcpy_cap(*fatal_error, 512, "Error: not a .dds file: ");
        T1_std_strcat_cap(*fatal_error, 512, filename); 
        return;
    }
    
    T1_tex_array_reg_img(
        /* const char * filename: */
            filename,
        /* const u32 width: */
            header->width,
        /* const u32 height: */
            header->height,
        /* const u32 is_render_target: */
            false,
        /* const u32 is_dds_image: */
            true,
        /* fatal_error: */
            fatal_error);
    
    T1_mem_free_managed(contents);
}

void T1_tex_files_decode_all_prereg(
    const u32 thread_id,
    const u32 using_num_threads,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    if (!T1_tex_arrays) { return; }
    
    s32 texture_arrays_to_init = (s32)T1_tex_arrays_size - 1;
    s32 texture_arrays_per_thread =
        texture_arrays_to_init / (s32)using_num_threads;
    if (texture_arrays_per_thread < 1) {
        texture_arrays_per_thread = 1;
    }
    
    s32 start_ta_i = 1 + ((s32)thread_id * texture_arrays_per_thread);
    s32 end_ta_i =
        thread_id + 1 >= using_num_threads ?
            (s32)T1_tex_arrays_size :
            start_ta_i + texture_arrays_per_thread;
    
    T1_log_append("Thread ");
    T1_log_append_u32(thread_id);
    T1_log_append("/");
    T1_log_append_u32(using_num_threads);
    T1_log_append(" decoding texarrays ");
    T1_log_append_s32(start_ta_i);
    T1_log_append(" - ");
    T1_log_append_s32(end_ta_i);
    T1_log_append("...\n");
    
    T1_log_assert(using_num_threads > 0);
    T1_log_assert(using_num_threads < 7);
    
    for (s32 ta_i = start_ta_i; ta_i < end_ta_i; ta_i++) {
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
            u32 t_i = 0;
            t_i < T1_tex_arrays[ta_i].images_size;
            t_i++)
        {
            if (
                T1_tex_arrays[ta_i].images[t_i].name[0] == '\0' ||
                T1_tex_arrays[ta_i].images[t_i].image.
                    rgba_values_freeable != NULL ||
                T1_tex_arrays[ta_i].images[t_i].image.
                    rgba_values_size == 0)
            {
                continue;
            }
            
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
                /* const u32 thread_id: */
                    thread_id,
                    fatal_error);
            
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
