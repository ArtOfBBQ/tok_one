#include "platform_layer.h"

void resource_filename_to_pathfile(
    const char * filename,
    char * recipient,
    const uint32_t assert_capacity)
{
    uint32_t filename_length =
        get_string_length(filename);
    uint32_t separator_size =
        platform_get_directory_separator_size();
    char separator[separator_size + 1];
    platform_get_directory_separator(
        /* recipient: */ separator);
    
    char separator_and_filename[
        filename_length + separator_size + 1];
    concat_strings(
        /* char * string_1            : */
            separator,
        /* char * string_2            : */
            filename,
        /* char * output              : */
            separator_and_filename,
        /* const uint64_t output_size : */
            filename_length + separator_size + 1);
    
    char * resource_path =
        platform_get_resources_path();
    uint32_t resource_path_length =
        get_string_length(resource_path);
    
    uint32_t full_filename_size =
        (filename_length
            + resource_path_length
            + 2); // +1 for \0, +1 to add a '/'
    log_assert(assert_capacity >= full_filename_size);
    if (!application_running) {
        recipient[0] = '\0';
        return;
    }
    
    concat_strings(
        /* char * string_1            : */
            resource_path,
        /* char * string_2            : */
            separator_and_filename,
        /* char * output              : */
            recipient,
        /* const uint32_t output_size : */
            full_filename_size);
};

