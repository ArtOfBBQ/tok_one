#define SHARED_APPLE_PLATFORM
#define PLATFORM_NS_FILEMANAGER

#include "../shared/platform_layer.h"

uint64_t platform_get_current_time_microsecs()
{
    uint64_t result = mach_absolute_time() / 1000;
    
    return result;
}

bool32_t platform_file_exists(const char * filename)
{
    return false;
}

void platform_mkdir_if_not_exist(const char * dirname)
{
    return;
}

void platform_delete_file(
    const char * filename)
{
    
}

void platform_write_file(
    const char * filepath_destination,
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
    *recipient_size = 0;
}

char * platform_get_application_path() {
    return (char *)
        [[[NSBundle mainBundle] resourcePath] UTF8String];
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
