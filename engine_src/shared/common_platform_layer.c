#include "platform_layer.h"

#define MAX_FILENAME_SIZE  512
#define MAX_SEPARATOR_SIZE   3 // 2 characters and NULL terminator

/*
Get a file's size. Returns -1 if no such file

same as platform_get_filesize() except it assumes
the resources directory
*/
uint64_t platform_get_resource_size(const char * filename) {
    char pathfile[500];
    resource_filename_to_pathfile(
        filename,
        /* recipient: */ pathfile,
        /* assert_capacity: */ 500);
    
    return platform_get_filesize(pathfile);
}

void platform_write_file_to_writables(
    const char * filepath_inside_writables,
    const char * output,
    const uint32_t output_size,
    bool32_t * good)
{
    char recipient[500];
    writable_filename_to_pathfile(
        /* filename: */
            filepath_inside_writables,
        /* recipient: */
            recipient,
        /* recipient_capacity: */
            500);
    
    platform_write_file(
        /* const char * filepath: */
            recipient,
        /* const char * output: */
            output,
        /* const uint32_t output_size: */
            output_size,
        /* bool32_t * good: */
            good);
}


bool32_t platform_resource_exists(const char * resource_name) {
    char pathfile[500];
    resource_filename_to_pathfile(
        resource_name,
        /* recipient: */ pathfile,
        /* capacity: */ 500);
    
    return platform_file_exists(
        /* filepath: */ pathfile);
}

void platform_read_resource_file(
    const char * filename,
    FileBuffer * out_preallocatedbuffer)
{
    char pathfile[500];
    resource_filename_to_pathfile(
        filename,
        /* recipient: */ pathfile,
        /* capacity: */ 500);
    
    platform_read_file(
        /* filepath :*/
            pathfile,
        /* out_preallocatedbuffer: */
            out_preallocatedbuffer);
}

void resource_filename_to_pathfile(
    const char * filename,
    char * recipient,
    const uint32_t assert_capacity)
{
    #ifdef COMMON_IGNORE_ASSERTS
    (void)assert_capacity;
    #endif
    
    assert(filename != NULL);
    assert(recipient != NULL);
    
    char resource_path[256];
    platform_get_resources_path(resource_path, 256);
    strcpy_capped(
        recipient,
        assert_capacity,
        resource_path);
    // uint32_t separator_size = platform_get_directory_separator_size();
    char separator[MAX_SEPARATOR_SIZE];
    platform_get_directory_separator(
        /* recipient: */ separator);
    strcat_capped(
        recipient,
        assert_capacity,
        separator);
    strcat_capped(
        recipient,
        assert_capacity,
        filename);
    
    assert(recipient[0] != '\0');
}

void writable_filename_to_pathfile(
    const char * filename,
    char * recipient,
    const uint32_t assert_capacity)
{
    #ifdef COMMON_IGNORE_ASSERTS
    (void)assert_capacity;
    #endif
    
    assert(filename != NULL);
    assert(recipient != NULL);
    
    #ifndef COMMON_IGNORE_ASSERTS
    uint32_t separator_size = platform_get_directory_separator_size();
    #endif
    
    #if !defined(LOGGER_IGNORE_ASSERTS) || !defined(COMMON_IGNORE_ASSERTS)
    uint32_t filename_length = get_string_length(filename);
    #endif
    
    char separator[MAX_SEPARATOR_SIZE];
    platform_get_directory_separator(/* recipient: */ separator);
    
    log_assert(
        (filename_length + MAX_SEPARATOR_SIZE + 1) < MAX_FILENAME_SIZE);
    char separator_and_filename[MAX_FILENAME_SIZE];
    strcpy_capped(
        separator_and_filename,
        filename_length + separator_size + 1,
        separator);
    strcat_capped(
        separator_and_filename,
        filename_length + separator_size + 1,
        filename);
    
    char writables_path[256];
    platform_get_writables_path(writables_path, 256);
    
    #ifndef COMMON_IGNORE_ASSERTS
    uint32_t writables_path_length = get_string_length(writables_path);
    uint32_t full_filename_size =
        (filename_length
            + writables_path_length
            + 2); // +1 for \0, +1 to add a '/'
    log_assert(assert_capacity >= full_filename_size);
    if (!application_running) {
        recipient[0] = '\0';
        return;
    }
    #endif
    
    strcpy_capped(
        recipient,
        assert_capacity,
        writables_path);
    strcat_capped(
        recipient,
        assert_capacity,
        separator_and_filename);
    
    assert(recipient[0] != '\0');
}

void platform_delete_writable(
    const char * writable_filename)
{
    char filepath[500];
    
    writable_filename_to_pathfile(
        /* const char * filename: */
            writable_filename,
        /* char * recipient: */
            filepath,
        /* const uint32_t assert_capacity: */
            500);
    
    platform_delete_file(writable_filename);
}

void platform_assert_mutex_locked(const uint32_t mutex_id) {
    log_assert(!platform_mutex_trylock(mutex_id));
}
