#define SHARED_APPLE_PLATFORM

#include "../shared/platform_layer.h"

uint64_t start_time = 0;

uint64_t platform_get_current_time_nanosecs()
{
    return mach_absolute_time();
}
