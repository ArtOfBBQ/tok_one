#include "T1_platform_layer.h"

uint32_t T1_platform_get_dir_separator_size(void) {
    return 1;
}

void T1_platform_get_dir_separator(char * recipient) {
    recipient[0] = '/';
    recipient[1] = '\0';
}

uint64_t
T1_platform_get_current_time_us(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    uint64_t result =
        1000000 *
            (uint64_t)tv.tv_sec +
            (uint64_t)tv.tv_usec;
    
    return result;
}

uint64_t
T1_platform_get_clock_frequency(void) {
    //    int mib[2];
    //    size_t len;
    //    mib[0] = CTL_KERN;
    //    mib[1] = KERN_CLOCKRATE;
    //    struct clockinfo clockinfo;
    //    len = sizeof(clockinfo);
    //    int result = sysctl(mib, 2, &clockinfo, &len, NULL, 0);
    //    assert(result != -1);
    // log_trace("clockinfo.hz: %d\n", clockinfo.hz);
    // log_trace("clockinfo.tick: %d\n", clockinfo.tick);
    // return (uint64_t)clockinfo.tick;
    return 3600000000; // my pc's clock frequency
}

#if T1_AUDIO_ACTIVE == T1_ACTIVE
void
T1_platform_audio_start_loop(void)
{
    T1_apple_audio_start_loop();
}
#elif T1_AUDIO_ACTIVE == T1_INACTIVE
#else
#error
#endif

/*
Get a file's size. Returns 0 if no such file
*/
uint64_t
T1_platform_get_filesize(
    const char * filepath)
{
    uint64_t return_value;
    
    NSString * nsfilepath = [NSString
        stringWithCString:filepath
        encoding:NSASCIIStringEncoding];
    
    NSError * error_value = nil;
    
    NSFileManager * file_manager = [NSFileManager defaultManager];
    
    if (file_manager == nil) {
        T1_log_append("ERROR - failed to get default NSFileManager\n");
        return 0;
    }
    
    NSDictionary<NSString *,id> * attrib_dict = [file_manager
        attributesOfItemAtPath:nsfilepath
        error:&error_value];
    
    if (error_value != nil) {
        T1_log_append("ERROR - failed to get size of file: ");
        T1_log_append(filepath);
        T1_log_append("\n");
        return 0;
    }
    
    uint64_t file_size = (uint64_t)[attrib_dict fileSize];
    
    return_value = file_size;
    
    return return_value;
}

void T1_platform_read_file(
    const char * filepath,
    T1FileBuffer * out_preallocatedbuffer)
{
    //@autoreleasepool {
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
    
    if (
        error ||
        file_data == nil ||
        out_preallocatedbuffer->size_without_terminator < 1 ||
        [file_data length] < out_preallocatedbuffer->
            size_without_terminator)
    {
        T1_log_append("Error - failed [NSData initWithContentsOfFile:]\n");
        out_preallocatedbuffer->size_without_terminator = 0;
        out_preallocatedbuffer->good = false;
        return;
    }
    
    [file_data
        getBytes:
            out_preallocatedbuffer->contents
        length:
            out_preallocatedbuffer->
                size_without_terminator];
    
    out_preallocatedbuffer->contents[
        out_preallocatedbuffer->
            size_without_terminator] = '\0';
    
    out_preallocatedbuffer->good = true;
}

bool32_t T1_platform_file_exists(
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
        T1_log_append("warning filepath: ");
        T1_log_append(filepath);
        T1_log_append(" is a directory, returnin FALSE for existence\n");
            return false;
        }
        
        return true;
    }
    
    T1_log_append("filepath: ");
    T1_log_append(filepath);
    T1_log_append(" does not exist, returning FALSE...\n");
    return false;
}

void T1_platform_mkdir_if_not_exist(
    const char * dirname)
{
    T1_log_append("make directory if it doesn't exist: ");
    T1_log_append(dirname);
    T1_log_append("\n");
    
    NSString * directory_path = [NSString
        stringWithCString:dirname
        encoding:NSASCIIStringEncoding];
    
    #ifndef NDEBUG
    NSURL * directory_url = [NSURL
        fileURLWithPath: directory_path
        isDirectory: true];
    assert(directory_url != NULL);
    #endif
    
    if (
        ![[NSFileManager defaultManager]
            fileExistsAtPath:directory_path])
    {
        NSError * error = NULL;
        
        bool32_t success = (bool32_t)[[NSFileManager defaultManager]
            createDirectoryAtPath:directory_path
            withIntermediateDirectories:true
            attributes:NULL 
            error:&error];
        
        if (!success) {
            T1_log_append("ERROR - tried to create a directory and failed\n");
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

void T1_platform_del_file(
    const char * filepath)
{
    T1_log_append(
        "trying to delete a file with "
        "NSFileManager: ");
    T1_log_append(filepath);
    T1_log_append("\n");
    NSString * nsfilepath = [NSString
        stringWithCString:filepath
        encoding:NSASCIIStringEncoding];
    
    [[NSFileManager defaultManager]
        removeItemAtPath: nsfilepath
        error: nil];
}

void T1_platform_copy_file(
    const char * filepath_source,
    const char * filepath_destination)
{
    T1_log_append("trying to copy from: ");
    T1_log_append(filepath_source);
    T1_log_append(", to: ");
    T1_log_append(filepath_destination);
    T1_log_append_char('\n');
    
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
T1_platform_write_file(
    const char * filepath,
    const char * output,
    const uint32_t output_size,
    bool32_t * good)
{
    T1_log_append("write file data to: ");
    T1_log_append(filepath);
    T1_log_append("\n");
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
        T1_log_append("Failed to write to file: ");
        T1_log_append(filepath);
        *good = false;
        return;
    }
    
    *good = true;
}

void T1_platform_get_filenames_in(
    const char * directory,
    char filenames[2000][500])
{
    NSString * path = [NSString
        stringWithCString:directory
        encoding:NSASCIIStringEncoding];
    // NSURL * url = [NSURL URLWithString: path];
    
    // T1_log_assert(url != NULL);
    NSError * error = NULL;
    
    NSArray * results = [[NSFileManager defaultManager]
        contentsOfDirectoryAtPath:path
        error: &error];
    
    if (error != NULL) {
        NSLog(@" error => %@ ", [error userInfo]);
        return;
    }
    
    uint32_t storable_results =
        (uint32_t)[results count] > 2000 ?
            2000 : (uint32_t)[results count];
    
    for (
        uint32_t i = 0;
        i < storable_results;
        i++)
    {
        NSString * current_result = [results[i] lastPathComponent];
        
        T1_std_strcpy_cap(
            filenames[i],
            500,
            (char *)[current_result
                cStringUsingEncoding:NSASCIIStringEncoding]);
    }
}

void
T1_platform_get_app_dir(
    char * recipient,
    const uint32_t recipient_size)
{
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    (void)recipient_size;
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_std_strcpy_cap(
        recipient,
        recipient_size,
        (char *)[[[NSBundle mainBundle] bundlePath]
            cStringUsingEncoding:
                NSASCIIStringEncoding]);
}

void T1_platform_get_res_dir(
    char * recipient,
    const uint32_t recipient_size)
{
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    (void)recipient_size;
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_std_strcpy_cap(
        recipient,
        recipient_size,
        (char *)[
            [[NSBundle mainBundle] resourcePath]
                cStringUsingEncoding: NSASCIIStringEncoding]);
}

void T1_platform_start_thread(
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
    // T1_log_assert(result == 0);
    
    dispatch_async(
        dispatch_get_global_queue(
            DISPATCH_QUEUE_PRIORITY_BACKGROUND,
            0),
        ^{
            function_to_run(argument);
        });
}

uint32_t T1_platform_get_cpu_logical_core_count(void)
{
    NSUInteger core_count = [
        [NSProcessInfo processInfo] activeProcessorCount];
    return (core_count > 0) ? (unsigned int)core_count : 1;
}
