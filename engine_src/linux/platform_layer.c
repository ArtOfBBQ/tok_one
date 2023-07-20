#include "../shared/platform_layer.h"

void platform_get_cwd(
    char * recipient,
    const uint32_t recipient_size)
{
    strcpy_capped(
        recipient,
        recipient_size,
        "./");
}

uint64_t platform_get_current_time_microsecs()
{
    return 1;
}

void platform_get_application_path(
    char * recipient,
    const uint32_t recipient_size)
{
    strcpy_capped(
        recipient,
        recipient_size,
        "./");
}

void platform_start_thread(
    void (*function_to_run)(int32_t),
    int32_t argument)
{
    
}

