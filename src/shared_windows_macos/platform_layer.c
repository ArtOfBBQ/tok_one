#include "../shared/platform_layer.h"

/*
Get a file's size. Returns -1 if no such file
*/
int64_t platform_get_filesize(const char * filename)
{
    char filename_with_slash[1000];
    char path_and_filename[1000];
    
    concat_strings(
        /* string_1: */
            "/",
        /* string_2: */
            filename,
        /* output: */
            filename_with_slash,
        /* output_size: */
            1000);
    
    concat_strings(
        /* string_1: */
            platform_get_application_path(),
        /* string_2: */
            filename_with_slash,
        /* output: */
            path_and_filename,
        /* output_size: */
            1000);
    
    FILE * file_handle = fopen(
        path_and_filename,
        "rb+");
    
    if (!file_handle) {
        printf("file read unsuccesful!\n");
        return -1;
    }
    
    fseek(file_handle, 0L, SEEK_END);
    int64_t fsize = ftell(file_handle);
    printf(
        "finished platform_get_filesize(), fsize: %i\n",
        fsize);
    
    fclose(file_handle);
    
    return fsize;
}

/*
Read some text from a file given a filename.

This function is here because it can't be used on iOS.
*/
void platform_read_file(
    const char * filename,
    FileBuffer * out_preallocatedbuffer)
{
    printf(
        "platform_read_file: %s of size: %u\n",
        filename,
        out_preallocatedbuffer->size);
    assert(out_preallocatedbuffer != NULL);
    assert(out_preallocatedbuffer->size > 0);
    assert(out_preallocatedbuffer->contents != NULL);
    printf(
        "platform_read_file: %s into buffer sized: %lu\n",
        filename,
        out_preallocatedbuffer->size);
    char filename_with_slash[1000];
    char path_and_filename[1000];
    
    printf("concat_strings...\n"); 
    concat_strings(
        /* string_1: */
            "/",
        /* string_2: */
            filename,
        /* output: */
            filename_with_slash,
        /* output_size: */
            1000);
    printf("filename_with_slash: %s\n", filename_with_slash);
    concat_strings(
        /* string_1: */
            platform_get_application_path(),
        /* string_2: */
            filename_with_slash,
        /* output: */
            path_and_filename,
        /* output_size: */
            1000);
    printf("path_and_filename: %s\n", path_and_filename);
   
    sleep(0.25f); 
    FILE * file_handle = fopen(
        path_and_filename,
        "rb+");
    
    if (!file_handle) {
        printf("file read unsuccesful!\n");
        return NULL;
    }
    
    fseek(file_handle, 0L, SEEK_END);
    long int fsize = ftell(file_handle);
    fseek(file_handle, 0L, SEEK_SET);
    
    // could be that the file size is smaller than what we need
    // we're wasting memory if so, so try to avoid
    // we're using fsize+1 and not fsize because we'll append a 0
    // at the end of the buffer's contents
    out_preallocatedbuffer->size =
        (fsize + 1) < out_preallocatedbuffer->size ?
            (fsize + 1)
            : out_preallocatedbuffer->size;
    printf(
        "changed out_preallocatedbuffer->size to: %u\n",
        out_preallocatedbuffer->size);
    
    printf("fread:\n");
    size_t bytes_read = fread(
        /* ptr: */
            out_preallocatedbuffer->contents,
        /* size of each element to be read: */
            1,
        /* nmemb (no of members) to read: */
            out_preallocatedbuffer->size - 1,
        /* stream: */
            file_handle);
    printf("fread succesful:\n");
   
    printf("fclose\n"); 
    fclose(file_handle);
    printf("fclose succesful\n"); 

    printf("append a 0 at the end\n"); 
    out_preallocatedbuffer->contents[
        out_preallocatedbuffer->size - 1]
            = 0; // for windows
    
    printf(
        "finished platform_read_file: %s into buffer sized: %u\n",
        filename,
        out_preallocatedbuffer->size);
    
    return out_preallocatedbuffer;
}

