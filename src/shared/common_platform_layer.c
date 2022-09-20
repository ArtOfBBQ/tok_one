#include "platform_layer.h"

void writable_filename_to_pathfile(
    const char * filename,
    char * recipient,
    const uint32_t assert_capacity)
{
    uint32_t filename_length = get_string_length(filename);
    uint32_t separator_size = platform_get_directory_separator_size();
    char separator[separator_size + 1];
    platform_get_directory_separator(/* recipient: */ separator);
    
    char separator_and_filename[filename_length + separator_size + 1];
    strcpy_capped(
        separator_and_filename,
        filename_length + separator_size + 1,
        separator);
    strcat_capped(
        separator_and_filename,
        filename_length + separator_size + 1,
        filename);
    
    char * writables_path = platform_get_writables_path();
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
    
    strcpy_capped(
        recipient,
        assert_capacity,
        writables_path);
    strcat_capped(
        recipient,
        assert_capacity,
        separator_and_filename);
};


void resource_filename_to_pathfile(
    const char * filename,
    char * recipient,
    const uint32_t assert_capacity)
{
    char * resource_path = platform_get_resources_path();
    strcpy_capped(
        recipient,
        assert_capacity,
        resource_path);
    uint32_t separator_size = platform_get_directory_separator_size();
    char separator[separator_size + 1];
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
};

void platform_delete_writable(const char * writable_filename) {
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

