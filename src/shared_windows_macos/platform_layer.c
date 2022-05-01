#include "../shared/platform_layer.h"

/*
Read some text from a file given a filename.

This function is here because it can't be used on iOS.
*/
FileBuffer * platform_read_file(
    const char * filename)
{
    char * filename_with_slash =
        concat_strings("/", filename);
    char * path_and_filename =
        concat_strings(
            platform_get_application_path(),
            filename_with_slash);
    
    printf("platform_read_file(%s)\n", path_and_filename);
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
        printf("bytes read mismatched fsize, freeing...\n");
        free(file_handle);
        free(return_value);
        free(buffer);
        printf("done freeing memory, returning nullptr\n");
        return NULL;
    }
    
    return_value->contents = buffer;
    return_value->contents[fsize] = 0; // for windows
    return_value->size = bytes_read;
    
    // if (file_handle) { 
    //     printf("freeing file handle\n"); 
    //     free(file_handle);
    // }
    
    printf(
        "finished platform_read_file(%s) - %u bytes\n",
        path_and_filename,
        return_value->size);
    
    return return_value;
}

