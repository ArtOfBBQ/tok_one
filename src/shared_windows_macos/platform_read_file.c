#include "../shared/platform_layer.h"

/*
Read some text from a file given a filename.

This function is here because it can't be used on iOS.
*/
FileBuffer * platform_read_file(char * filename) {
    printf("reading file: %s\n", filename);
    
    FILE * file_handle = fopen(
        filename,
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
        return NULL;
    }
    
    return_value->contents = buffer;
    return_value->contents[fsize] = 0; // for windows
    return_value->size = bytes_read;

    printf("read file of %lu bytes\n", bytes_read);
    
    return return_value;
}

