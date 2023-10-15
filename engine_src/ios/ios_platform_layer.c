#define PLATFORM_IOS

#include "../shared/platform_layer.h"

void * platform_malloc_unaligned_block(
    const uint64_t size)
{
    void * return_value = malloc(size);
    
    return return_value;
}

void platform_close_application(void) {
    log_append("Won't close app on iOS...\n");
}

float platform_x_to_x(const float x) {
    return x;
}

// 5 should become about 635,
// 635 should become about 5
float platform_y_to_y(const float y) {
    return (window_globals->window_height - y);
}

void platform_open_folder_in_window_if_possible(
    const char * folderpath)
{
    (void)folderpath;
    
    log_append("Ignoring folder open request - impossible on iOS\n");
}
