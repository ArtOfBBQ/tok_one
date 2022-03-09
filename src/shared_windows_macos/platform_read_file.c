#include "../shared/platform_layer.h"

/*
Read some text from a file given a filename.

This function is here because it can't be used on iOS.
*/
FileBuffer * platform_read_file(char * filename) {
    printf("reading file: %s\n", filename);
    
    
    FILE * file_handle = fopen(
        filename,
        "rb");

    if (!file_handle) {
        printf("file read unsuccesful!\n");
        return NULL;
    }

    FileBuffer * return_value = malloc(sizeof(FileBuffer));
    
    fseek(file_handle, 0, SEEK_END);
    unsigned long fsize = (unsigned long)ftell(file_handle);
    fseek(file_handle, 0, SEEK_SET);
    
    char * buffer = malloc(fsize);
    
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
        return NULL;
    }
    
    return_value->contents = buffer;
    return_value->size = bytes_read;
    
    return return_value;
}

