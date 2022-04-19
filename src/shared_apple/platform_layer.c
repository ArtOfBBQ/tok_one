#define SHARED_APPLE_PLATFORM

#include "../shared/platform_layer.h"

uint64_t platform_get_current_time_microsecs()
{
    uint64_t result = mach_absolute_time() / 1000;
    
    return result;
}
