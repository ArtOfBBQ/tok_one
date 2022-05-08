#include "../shared/platform_layer.h"

/*
Read some text from a file given a filename.

This function is here because it can't be used on iOS.
*/
FileBuffer * platform_read_file(
    const char * filename)
{
    char filename_with_slash[5000];
    char path_and_filename[5000];
    
    concat_strings(
        /* string_1: */ "/",
        /* string_2: */ filename,
        /* output: */ filename_with_slash,
        /* output_size: */ 5000);
    concat_strings(
        /* string_1: */ platform_get_application_path(),
        /* string_2: */ filename_with_slash,
        /* output: */ path_and_filename,
        /* output_size: */ 5000);
    
    FILE * file_handle = fopen(
        path_and_filename,
        "rb+");
    
    if (!file_handle) {
        printf("file read unsuccesful!\n");
        return NULL;
    }
    
    FileBuffer * return_value =
        (FileBuffer *)malloc(sizeof(FileBuffer));
    
    fseek(file_handle, 0L, SEEK_END);
    long int fsize = ftell(file_handle);
    fseek(file_handle, 0L, SEEK_SET);
    
    char * buffer = (char *)malloc(fsize + 1);
    
    size_t bytes_read = fread(
        /* ptr: */
            buffer,
        /* size of each element to be read: */
            1,
        /* nmemb (no of members) to read: */
            fsize,
        /* stream: */
            file_handle);
    
    fclose(file_handle);
    if (bytes_read != fsize) {
        free(file_handle);
        free(return_value);
        free(buffer);
        return NULL;
    }
    
    return_value->contents = buffer;
    return_value->contents[fsize] = 0; // for windows
    return_value->size = bytes_read;
    
    return return_value;
}

