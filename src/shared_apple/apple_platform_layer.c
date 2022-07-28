#define SHARED_APPLE_PLATFORM
#define PLATFORM_NS_FILEMANAGER

#include "../shared/platform_layer.h"

char * writables_path = NULL;

char * platform_get_writables_path(void) {
    
    if (writables_path == NULL) {
        
        NSArray * paths = NSSearchPathForDirectoriesInDomains(
            NSApplicationSupportDirectory,
            NSUserDomainMask,
            YES);
        
        NSString * libraryDirectory = [paths objectAtIndex:0];
        
        char * library_dir =
            (char *)[libraryDirectory
                cStringUsingEncoding: NSUTF8StringEncoding];
        
        // +2 because 1 for null terminator, 1 for an '/' in between
        writables_path = (char *)malloc_from_unmanaged(
            get_string_length(library_dir) + get_string_length(application_name) + 2);
        
        uint32_t i = 0;
        while (library_dir[i] != '\0') {
            writables_path[i] = library_dir[i];
            i++;
        }
        writables_path[i++] = '/';
        uint32_t j = 0;
        while (application_name[j] != '\0') {
            writables_path[i++] = application_name[j++];
        }
        writables_path[i] = '\0';
        
        platform_mkdir_if_not_exist(writables_path);
    }
    
    log_assert(writables_path != NULL);
    log_append("writables_path is: ");
    log_append(writables_path);
    log_append("\n");
    
    return writables_path;
}

uint32_t platform_get_directory_separator_size() {
    return 1;
}

void platform_get_directory_separator(char * recipient) {
    recipient[0] = '/';
    recipient[1] = '\0';
}

uint64_t __attribute__((no_instrument_function))
platform_get_current_time_microsecs(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    uint64_t result = 1000000 * tv.tv_sec + tv.tv_usec;
    
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
        out_preallocatedbuffer->size = 0;
        out_preallocatedbuffer->good = false;
        return;
    }
    
    [file_data
        getBytes: out_preallocatedbuffer->contents
        length: out_preallocatedbuffer->size];
    
    out_preallocatedbuffer->good = true;
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
            return false;
        }
        
        return true;
    }
    
    return false;
}

void platform_mkdir_if_not_exist(const char * dirname) {    
    
    NSString * directory_path = [NSString
        stringWithCString:dirname
        encoding:NSASCIIStringEncoding];
    NSURL * directory_url = [NSURL
        fileURLWithPath: directory_path
        isDirectory: true];
    assert(directory_url != NULL);
    
    if (![[NSFileManager defaultManager] fileExistsAtPath:directory_path])
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

void platform_copy_file(
    const char * filepath_source,
    const char * filepath_destination)
{
    NSString * nsfilepath_source = [NSString
        stringWithCString:filepath_source
        encoding:NSASCIIStringEncoding];
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

void __attribute__((no_instrument_function))
platform_write_file(
    const char * filepath,
    const char * output,
    const uint32_t output_size)
{
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
        log_append("\nPerhaps the operating system doesn't allow this app to write there?\n");
        assert(0);
    }
}

void platform_write_file_to_writables(
    const char * filepath_inside_writables,
    const char * output,
    const uint32_t output_size)
{
    char * writables_path = platform_get_writables_path();
    uint32_t writables_path_size = get_string_length(writables_path);
    uint32_t filepath_inside_writables_size =
        get_string_length(filepath_inside_writables);
    log_assert(
        writables_path_size
            + filepath_inside_writables_size
            + 1
                < 1000);
    
    char full_filepath[1000];
    
    uint32_t i = 0;
    while (i < writables_path_size) {
        full_filepath[i] = writables_path[i];
        i++;
    }
    full_filepath[i++] = '/';
    uint32_t j = 0;
    while (j < filepath_inside_writables_size) {
        full_filepath[i++] = filepath_inside_writables[j++];
    }
    full_filepath[i] = '\0';
    
    platform_write_file(
        /* const char * filepath: */
            full_filepath,
        /* const char * output: */
            output,
        /* const uint32_t output_size: */
            output_size);
}

void platform_get_filenames_in(
    const char * directory,
    char ** filenames,
    const uint32_t recipient_capacity,
    uint32_t * recipient_size)
{
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
        return;
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

char * __attribute__((no_instrument_function))
platform_get_application_path() {
    return (char *)
        [[[NSBundle mainBundle] bundlePath]
            cStringUsingEncoding: NSASCIIStringEncoding];
}

char * platform_get_resources_path() {
    return (char *)
        [[[NSBundle mainBundle] resourcePath] cStringUsingEncoding: NSASCIIStringEncoding];
}

void platform_start_thread(
    void (*function_to_run)(int32_t),
    int32_t argument)
{
    dispatch_async(
        dispatch_get_global_queue(
            DISPATCH_QUEUE_PRIORITY_BACKGROUND,
            0),
        ^{
            function_to_run(argument);
        });
}
