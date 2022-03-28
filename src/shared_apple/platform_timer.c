#define SHARED_APPLE_PLATFORM

#include "../shared/platform_layer.h"

uint64_t start_time = 0;

void platform_start_timer() {
    printf("%s\n", "start timer");
    
    start_time = mach_absolute_time();
}

uint64_t platform_end_timer_get_nanosecs()
{
    uint64_t end_time = mach_absolute_time();
    
    // time elapsed in "mach time units".
    const uint64_t elapsed_time = end_time - start_time;
    
    // get information for converting from MTU to nanoseconds
    mach_timebase_info_data_t info;
    if (mach_timebase_info(&info)) {
        printf("unhandled error - machtime.h\n");
        assert(0);
    }
    
    return (uint64_t)((double)elapsed_time *
        ((double)info.numer / (double)info.denom));
}

