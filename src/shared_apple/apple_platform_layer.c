#define SHARED_APPLE_PLATFORM
#define PLATFORM_NS_FILEMANAGER

#include "../shared/platform_layer.h"

uint32_t platform_get_directory_separator_size() {
    return 1;
}

void platform_get_directory_separator(
    char * recipient)
{
    recipient[0] = '/';
    recipient[1] = '\0';
}

uint64_t platform_get_current_time_microsecs()
{
    uint64_t result = mach_absolute_time() / 1000;
    
    return result;
}

/*
Get a file's size. Returns -1 if no such file

same as platform_get_filesize() except it assumes
the resources directory
*/
uint64_t platform_get_resource_size(
    const char * filename)
{
    char pathfile[500];
    resource_filename_to_pathfile(
        filename,
        /* recipient: */ pathfile,
        /* assert_capacity: */ 500);
    
    return platform_get_filesize(pathfile);
}

/*
Get a file's size. Returns 0 if no such file
*/
uint64_t platform_get_filesize(
    const char * filepath)
{
    NSString * nsfilepath = [NSString
        stringWithCString:filepath
        encoding:NSASCIIStringEncoding];
    
    
    NSError * error_value = nil;
    
    uint64_t file_size = (uint64_t)[
        [[NSFileManager defaultManager]
        attributesOfItemAtPath:nsfilepath
        error:&error_value] fileSize];
    
    if (error_value != nil)
    {
        NSLog(
            @" error => %@ ",
            [error_value userInfo]);
        return 0;
    }
    
    if (file_size < 1) {
        printf(
            "ERROR - failed to get file %s size for unknown reasons\n",
            filepath);
        assert(0);
    }
    
    // let's not use 20MB+ files in development
    assert(file_size < 20000000);
    
    return file_size;
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

void platform_read_file(
    const char * filepath,
    FileBuffer * out_preallocatedbuffer)
{
    printf(
        "platform_read_file: %s into buffer of size: " 
        FUINT64
        "\n",
        filepath,
        out_preallocatedbuffer->size);
    
    NSString * nsfilepath = [NSString
        stringWithCString:filepath
        encoding:NSASCIIStringEncoding];
    
    NSURL * file_url = [NSURL fileURLWithPath: nsfilepath];
    
    if (file_url == nil) {
        printf(
            "couldn't find file: %s\n",
            filepath);
        out_preallocatedbuffer->size = 0;
        out_preallocatedbuffer->good = false;
        return;
    }
    
    NSInputStream * input_stream = [NSInputStream
        inputStreamWithURL:file_url];
    [input_stream open];
    
    if (input_stream == nil) {
        printf("Error - failed to create NSInputStream from viable file NSURL\n");
        out_preallocatedbuffer->size = 0;
        out_preallocatedbuffer->good = false;
        assert(0); // TODO: remove this assert
        return;
    }
    
    NSInteger result =
        [input_stream
            read:
                (uint8_t *)out_preallocatedbuffer->contents
            maxLength:
                out_preallocatedbuffer->size - 1];
    
    if (result < 1) {
        NSError * stream_error = input_stream.streamError;
        
        if (stream_error != NULL) {
            NSLog(@" error => %@ ", [stream_error userInfo]);
        }
        
        out_preallocatedbuffer->size = 0;
        out_preallocatedbuffer->good = false;
        [input_stream close];
        
        return;
    }
    
    out_preallocatedbuffer->size = (uint64_t)result + 1;
    out_preallocatedbuffer->
        contents[out_preallocatedbuffer->size - 1] = '\0';
    out_preallocatedbuffer->good = true;
    [input_stream close];
}

bool32_t platform_file_exists(
    const char * filepath)
{
    NSString * nsfilepath = [NSString
        stringWithCString:filepath
        encoding:NSASCIIStringEncoding];
    NSURL * url = [NSURL URLWithString: nsfilepath];
    
    if (url != nil) {
        return true;
    }
    
    return false;
}

void platform_mkdir_if_not_exist(
    const char * dirname)
{
    return;
}

void platform_delete_file(
    const char * filepath)
{
    NSString * nsfilepath = [NSString
        stringWithCString:filepath
        encoding:NSASCIIStringEncoding];
    NSURL * file_url = [NSURL URLWithString: nsfilepath];
    
    NSError * error = NULL;
    if (file_url != nil) {
        [
            [NSFileManager defaultManager]
            removeItemAtURL:file_url
            error:&error];
    }
}

void platform_write_file(
    const char * filepath,
    const char * output)
{
    
}

void platform_copy_file(
    const char * filepath_source,
    const char * filepath_destination)
{
    
}

void platform_get_filenames_in(
    const char * directory,
    char ** filenames,
    const uint32_t recipient_capacity,
    uint32_t * recipient_size)
{
    printf("platform_get_filenames_in(): %s\n", directory);
    *recipient_size = 0;
    
    NSString * path = [NSString
        stringWithCString:directory
        encoding:NSASCIIStringEncoding];
    NSURL * url = [NSURL URLWithString: path];
    NSError * error = NULL;
    
    NSArray * results = [[NSFileManager defaultManager]
        contentsOfDirectoryAtURL:url
        includingPropertiesForKeys: nil
        options: NSDirectoryEnumerationSkipsHiddenFiles
        error: &error];
    
    if (error != NULL) {
        NSLog(@" error => %@ ", [error userInfo]);
        assert(0);
    }
    
    uint32_t storable_results =
        (uint32_t)[results count] > recipient_capacity ?
            recipient_capacity
            : (uint32_t)[results count];
    
    for (
        uint32_t i = 0;
        i < storable_results;
        i++)
    {
        NSString * current_result =
            [results[i] lastPathComponent];
        
        filenames[i] =
            (char *)[current_result
                cStringUsingEncoding:NSASCIIStringEncoding];
        *recipient_size += 1;
    }
}

char * platform_get_application_path() {
    return (char *)
        [[[NSBundle mainBundle] bundlePath] UTF8String];
}

char * platform_get_resources_path() {
    return (char *)
        [[[NSBundle mainBundle] resourcePath] UTF8String];
}

void platform_start_thread(
    void (*function_to_run)(int32_t),
    int32_t argument)
{
    dispatch_async(
        dispatch_get_global_queue(
            DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0),
        ^{
            function_to_run(argument);
        });
}
