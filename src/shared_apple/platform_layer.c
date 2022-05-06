#define SHARED_APPLE_PLATFORM
#define PLATFORM_NS_FILEMANAGER

#include "../shared/platform_layer.h"

uint64_t platform_get_current_time_microsecs()
{
    uint64_t result = mach_absolute_time() / 1000;
    
    return result;
}

char * platform_get_application_path() {
    return (char *)[
        [[NSBundle mainBundle] resourcePath] UTF8String];
}

