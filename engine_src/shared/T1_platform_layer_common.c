#include "T1_platform_layer.h"

#include "decode_png.h"
#include "decode_bmp.h"
#include "T1_mem.h"
#include "T1_log.h"
#include "T1_global.h"
#include "T1_zsprite.h"
#include "T1_texquad.h"
#include "T1_ui_widget.h"


#define MAX_FILENAME_SIZE  512
#define MAX_SEPARATOR_SIZE   3 // 2 characters and NULL terminator

/*
Get a file's size. Returns -1 if no such file

same as platform_get_filesize() except it assumes
the resources directory
*/
u64 T1_os_get_resource_size(const char * filename)
{
    char pathfile[512];
    T1_os_res_filename_to_pathfile(
        filename,
        /* recipient: */ pathfile,
        /* assert_capacity: */ 512);
    
    return T1_os_get_filesize(pathfile);
}

u64 T1_os_get_writable_size(const char * filename) {
    char pathfile[512];
    T1_os_writable_filename_to_pathfile(
        filename,
        /* recipient: */ pathfile,
        /* assert_capacity: */ 512);
    
    return T1_os_get_filesize(pathfile);
}

void T1_os_read_file_from_writables(
    const char * filepath_in_writables,
    char * recip,
    const u32 recip_size,
    u8 * good)
{
    *good = 0;
    
    char filepath[512];
    T1_os_writable_filename_to_pathfile(
        /* filename: */
            filepath_in_writables,
        /* recipient: */
            filepath,
        /* recipient_capacity: */
            512);
    
    u32 bytes_read = 0;
    T1_os_read_file(
        /* const char * filepath: */
            filepath,
        /* char * recip: */
            recip,
            &bytes_read,
        /* u64 * size_without_term: */
            recip_size,
        /* u8 * good: */
            good);
    
    if (bytes_read != recip_size) {
        T1_log_assert(0);
        *good = 0;
        return;
    }
    
    *good = 1;
}

void T1_os_write_file_to_writables(
    const char * filepath_inside_writables,
    const char * output,
    const u32 output_size,
    u8 * good)
{
    char recipient[500];
    T1_os_writable_filename_to_pathfile(
        /* filename: */
            filepath_inside_writables,
        /* recipient: */
            recipient,
        /* recipient_capacity: */
            500);
    
    T1_os_write_file(
        /* const char * filepath: */
            recipient,
        /* const char * output: */
            output,
        /* const u32 output_size: */
            output_size,
        /* bool32_t * good: */
            good);
}

void
T1_os_write_rgba_to_writables(
    const char * local_filename,
    u8 * rgba,
    const u32 rgba_size,
    const u32 width,
    const u32 height,
    u8 * good)
{
    u32 bmp_cap = rgba_size + 10000;
    u8 * bmp = T1_mem_malloc_managed(bmp_cap);
    u32 bmp_size = 0;
    
    for (
        u32 rgba_i = 0;
        rgba_i < (width * height * 4);
        rgba_i += 4)
    {
        if (
            rgba[rgba_i+3] == 0)
        {
            // Use an ugly purple debug color for transparent pixels
            rgba[rgba_i+0] = 125;
            rgba[rgba_i+1] =   0;
            rgba[rgba_i+2] = 125;
        }
    }
    
    encode_BMP(
        /* const u8 * rgba :*/
            rgba,
        /* const u64 rgba_size: */
            rgba_size,
        /* const u32 width: */
            width,
        /* const u32 height: */
            height,
        /* char * recipient: */
            (char *)bmp,
        /* u32 * recipient_size: */
            &bmp_size,
        /* const int64_t recipient_capacity: */
            bmp_cap);
    
    T1_os_write_file_to_writables(
        /* const char * filepath_inside_writables: */
            local_filename,
        /* const char * output: */
            (const char *)bmp,
        /* const u32 output_size: */
            bmp_size,
        /* u32 * good: */
            good);
}


u8 T1_os_res_exists(const char * resource_name) {
    char pathfile[500];
    T1_os_res_filename_to_pathfile(
        resource_name,
        /* recipient: */ pathfile,
        /* capacity: */ 500);
    
    return T1_os_file_exists(
        /* filepath: */ pathfile);
}

void T1_os_read_resource_file(
    const char * filename,
    char * recip,
    const u64 recip_cap,
    u8 * good)
{
    char pathfile[500];
    T1_os_res_filename_to_pathfile(
        filename,
        /* recipient: */ pathfile,
        /* capacity: */ 500);
    
    u32 bytes_read = 0;
    T1_os_read_file(
        /* filepath :*/
            pathfile,
        /* out_preallocatedbuffer: */
            recip,
            &bytes_read,
        /* const u64 recip_cap :*/
            recip_cap,
        /* bool8_t * good: */
            good);
    
    if (bytes_read != recip_cap) {
        T1_log_assert(0);
        *good = 0;
        return;
    }
    
    *good = 1;
}


void T1_os_res_filename_to_pathfile(
    const char * filename,
    char * recipient,
    const u32 assert_capacity)
{
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    // pass
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    (void)assert_capacity;
    #else
    #error
    #endif
    
    assert(filename != NULL);
    assert(recipient != NULL);
    
    char resource_path[256];
    T1_os_get_res_dir(resource_path, 256);
    T1_std_strcpy_cap(
        recipient,
        assert_capacity,
        resource_path);
    // u32 separator_size = platform_get_directory_separator_size();
    char separator[MAX_SEPARATOR_SIZE];
    T1_os_get_dir_separator(
        /* recipient: */ separator);
    T1_std_strcat_cap(
        recipient,
        assert_capacity,
        separator);
    T1_std_strcat_cap(
        recipient,
        assert_capacity,
        filename);
    
    assert(recipient[0] != '\0');
}

void T1_os_writable_filename_to_pathfile(
    const char * filename,
    char * recipient,
    const u32 assert_capacity)
{
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    // pass
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    (void)assert_capacity;
    #else
    #error
    #endif
    
    assert(filename != NULL);
    assert(recipient != NULL);
    
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    u32 separator_size = T1_os_get_dir_separator_size();
    u32 filename_length = (u32)T1_std_strlen(filename);
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    char separator[MAX_SEPARATOR_SIZE];
    T1_os_get_dir_separator(/* recipient: */ separator);
    
    char separator_and_filename[MAX_FILENAME_SIZE];
    T1_std_strcpy_cap(
        separator_and_filename,
        filename_length + separator_size + 1,
        separator);
    T1_std_strcat_cap(
        separator_and_filename,
        filename_length + separator_size + 1,
        filename);
    
    char writables_path[256];
    T1_os_get_writables_dir(writables_path, 256);
    
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    u32 writables_path_length =
        (u32)T1_std_strlen(writables_path);
    u32 full_filename_size =
        (filename_length
            + writables_path_length
            + 2); // +1 for \0, +1 to add a '/'
    T1_log_assert(assert_capacity >= full_filename_size);
    if (!T1_log_app_running) {
        recipient[0] = '\0';
        return;
    }
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_std_strcpy_cap(
        recipient,
        assert_capacity,
        writables_path);
    T1_std_strcat_cap(
        recipient,
        assert_capacity,
        separator_and_filename);
    
    T1_log_assert(recipient[0] != '\0');
}

void T1_os_del_writable(
    const char * writable_filename)
{
    char filepath[500];
    
    T1_os_writable_filename_to_pathfile(
        /* const char * filename: */
            writable_filename,
        /* char * recipient: */
            filepath,
        /* const u32 assert_capacity: */
            500);
    
    T1_os_del_file(writable_filename);
}

void T1_os_layer_start_window_resize(
    const u64 timestamp)
{
    T1_global->last_resize_request_us = timestamp;
    
    T1_ui_widget_delete_all();
    T1_zsprite_delete_all();
    T1_texquad_delete_all();
    
    #if T1_FOG_ACTIVE == T1_ACTIVE
    T1_global->postproc_consts.fog_factor = 0.0f;
    T1_global->postproc_consts.fog_color[0] = 0.0f;
    T1_global->postproc_consts.fog_color[1] = 0.0f;
    T1_global->postproc_consts.fog_color[2] = 0.0f;
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
}
