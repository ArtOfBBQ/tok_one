#define SHARED_APPLE_PLATFORM
#define PLATFORM_NS_FILEMANAGER

#include "platform_layer.h"

void platform_get_writables_path(
    char * recipient,
    const uint32_t recipient_size)
{
    NSArray * paths = NSSearchPathForDirectoriesInDomains(
        NSApplicationSupportDirectory,
        NSUserDomainMask,
        YES);
    
    NSString * libraryDirectory = [paths objectAtIndex:0];
    
    char * library_dir =
        (char *)[libraryDirectory
            cStringUsingEncoding: NSUTF8StringEncoding];
    
    strcpy_capped(recipient, recipient_size, library_dir);
    strcat_capped(recipient, recipient_size, "/");
    strcat_capped(recipient, recipient_size, APPLICATION_NAME);
    
    platform_mkdir_if_not_exist(recipient);
}

uint32_t platform_get_directory_separator_size(void) {
    return 1;
}

void platform_get_directory_separator(char * recipient) {
    recipient[0] = '/';
    recipient[1] = '\0';
}

uint64_t __attribute__((no_instrument_function))
platform_get_current_time_microsecs(void) {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    uint64_t result =
        1000000 *
            (uint64_t)tv.tv_sec +
            (uint64_t)tv.tv_usec;
    
    return result;
}

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

/*
Get a file's size. Returns 0 if no such file
*/
uint64_t platform_get_filesize(const char * filepath) {
    
    uint64_t return_value;
    
    NSString * nsfilepath = [NSString
        stringWithCString:filepath
        encoding:NSASCIIStringEncoding];
    
    NSError * error_value = nil;
    
    uint64_t file_size = (uint64_t)[
        [[NSFileManager defaultManager]
        attributesOfItemAtPath:nsfilepath
        error:&error_value] fileSize];
    
    if (error_value != nil) {
        log_append("ERROR - failed to get size of file: ");
        log_append(filepath);
        log_append("\n");
        return 0;
    }
    
    return_value = file_size;
    
    return return_value;
}

void platform_read_file(
    const char * filepath,
    FileBuffer * out_preallocatedbuffer)
{
    @autoreleasepool {
    NSString * nsfilepath =
        [NSString
            stringWithCString:filepath
            encoding:NSASCIIStringEncoding];
    
    NSError * error = NULL;
    NSData * file_data =
        [NSData
            dataWithContentsOfFile: nsfilepath
            options: NSDataReadingUncached
            error: &error];
    
    if (error) {
        log_append("Error - failed [NSData initWithContentsOfFile:]\n");
        out_preallocatedbuffer->size_without_terminator = 0;
        out_preallocatedbuffer->good = false;
        return;
    }
    
    [file_data
        getBytes: out_preallocatedbuffer->contents
        length: out_preallocatedbuffer->size_without_terminator];
    
    out_preallocatedbuffer->contents[
        out_preallocatedbuffer->size_without_terminator] = '\0';
    
    out_preallocatedbuffer->good = true;
    }
}

bool32_t platform_file_exists(
    const char * filepath)
{
    NSString * nsfilepath = [NSString
        stringWithCString:filepath
        encoding:NSASCIIStringEncoding];
    
    BOOL is_directory = false;
    if ([[NSFileManager defaultManager]
        fileExistsAtPath: nsfilepath
        isDirectory: &is_directory])
    {
        if (is_directory) {
	    log_append("warning filepath: ");
	    log_append(filepath);
	    log_append(" is a directory, returnin FALSE for existence\n");
            return false;
        }
        
        return true;
    }
    
    log_append("filepath: ");
    log_append(filepath);
    log_append(" does not exist, returning FALSE...\n");
    return false;
}

void platform_mkdir_if_not_exist(const char * dirname) {    
    
    log_append("make directory if it doesn't exist: ");
    log_append(dirname);
    log_append("\n");
    
    NSString * directory_path = [NSString
        stringWithCString:dirname
        encoding:NSASCIIStringEncoding];
    NSURL * directory_url = [NSURL
        fileURLWithPath: directory_path
        isDirectory: true];
    assert(directory_url != NULL);
    
    if (
        ![[NSFileManager defaultManager]
            fileExistsAtPath:directory_path])
    {
        NSError * error = NULL;
        
        bool success = [[NSFileManager defaultManager]
            createDirectoryAtPath:directory_path
            withIntermediateDirectories:true
            attributes:NULL 
            error:&error];
        
        if (!success) {
            log_append("ERROR - tried to create a directory and failed\n");
            if (error != NULL) {
                NSLog(@" error => %@ ", [error userInfo]);
            }
        } else {
            assert([[NSFileManager defaultManager]
                fileExistsAtPath:directory_path]);
        }
    }
    
    return;
}

void platform_delete_file(const char * filepath) {
    
    log_append("trying to delete a file with NSFileManager: ");
    log_append(filepath);
    log_append("\n");
    NSString * nsfilepath = [NSString
        stringWithCString:filepath
        encoding:NSASCIIStringEncoding];
    
    [[NSFileManager defaultManager]
        removeItemAtPath: nsfilepath
        error: nil];
}

void platform_copy_file(
    const char * filepath_source,
    const char * filepath_destination)
{
    log_append("trying to copy from: ");
    log_append(filepath_source);
    log_append(", to: ");
    log_append(filepath_destination);
    log_append_char('\n');
    
    NSString * nsfilepath_source = [NSString
        stringWithCString:filepath_source
        encoding:NSASCIIStringEncoding];
    
    // if (platform_file_exists()f)
    
    NSString * nsfilepath_destination = [NSString
        stringWithCString:filepath_destination
        encoding:NSASCIIStringEncoding];
    
    NSError * error = NULL;
    
    [[NSFileManager defaultManager]
        copyItemAtPath: nsfilepath_source
        toPath: nsfilepath_destination
        error: &error];
    
    if (error != NULL) {
        NSLog(@" error => %@ ", [error userInfo]);
        assert(0);
    }
}

void
platform_write_file(
    const char * filepath,
    const char * output,
    const uint32_t output_size,
    bool32_t * good)
{
    log_append("write file data to: ");
    log_append(filepath);
    log_append("\n");
    NSString * nsfilepath = [NSString
        stringWithCString:filepath
        encoding:NSASCIIStringEncoding];
    
    NSData * nsdata = [NSData
        dataWithBytes:output
        length:output_size];
    
    if (![[NSFileManager defaultManager]
        createFileAtPath: 
            nsfilepath
        contents:
            nsdata
        attributes:
            nil])
    {
        log_append("Failed to write to file: ");
        log_append(filepath);
        *good = false;
        return;
    }
    
    *good = true;
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

void platform_get_filenames_in(
    const char * directory,
    char ** filenames,
    const uint32_t recipient_capacity,
    uint32_t * recipient_size)
{
    log_assert(recipient_capacity > 0);
    *recipient_size = 0;
    
    NSString * path = [NSString
        stringWithCString:directory
        encoding:NSASCIIStringEncoding];
    // NSURL * url = [NSURL URLWithString: path];
    
    // log_assert(url != NULL);
    // NSLog(@" url => %@ ", url);
    NSError * error = NULL;
    
    NSArray * results = [[NSFileManager defaultManager]
        contentsOfDirectoryAtPath:path
        error: &error];
    
    if (error != NULL) {
        NSLog(@" error => %@ ", [error userInfo]);
        return;
    }
    
    log_assert([results count] <= recipient_capacity);
    uint32_t storable_results =
        (uint32_t)[results count] > recipient_capacity ?
            recipient_capacity
            : (uint32_t)[results count];
    
    for (
        uint32_t i = 0;
        i < storable_results;
        i++)
    {
        NSString * current_result = [results[i] lastPathComponent];
        
        filenames[i] = (char *)[current_result
            cStringUsingEncoding:NSASCIIStringEncoding];
        *recipient_size += 1;
    }
}

void
platform_get_application_path(char * recipient, const uint32_t recipient_size) {
    strcpy_capped(
        recipient,
        recipient_size,
        (char *)[[[NSBundle mainBundle] bundlePath]
            cStringUsingEncoding: NSASCIIStringEncoding]);
}

void platform_get_resources_path(
    char * recipient,
    const uint32_t recipient_size)
{
    strcpy_capped(
        recipient,
        recipient_size,
        (char *)[
            [[NSBundle mainBundle] resourcePath]
                cStringUsingEncoding: NSASCIIStringEncoding]);
}

void platform_start_thread(
    void (*function_to_run)(int32_t),
    int32_t argument)
{
    // TODO: maybe we should just use pthread for threads instead of
    // dispatch_async, since we need pthreads for mutex locks anyway
    // Let's revisit this when we port to other platforms
    
    // pthread_t thread;
    // uint32_t result = pthread_create(
    //     &thread,
    //     NULL,
    //     function_to_run,
    //     argument);
    // log_assert(result == 0);
    
    dispatch_async(
        dispatch_get_global_queue(
            DISPATCH_QUEUE_PRIORITY_BACKGROUND,
            0),
        ^{
            function_to_run(argument);
        });
}
