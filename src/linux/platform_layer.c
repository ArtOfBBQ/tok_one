#include "../shared/platform_layer.h"

char * platform_get_cwd()
{
    return "./";
}

uint64_t platform_get_current_time_microsecs()
{
    return 1;
}

char * platform_get_application_path() {
    return "./";
}

void platform_start_thread(int32_t threadmain_id) {
}

