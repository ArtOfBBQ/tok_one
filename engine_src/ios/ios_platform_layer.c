#define PLATFORM_IOS

#include "../shared/platform_layer.h"

void platform_update_mouse_location(void) {
    // do nothing on iOS
}

void platform_toggle_fullscreen(void) {
    // do nothing on iOS
}

void platform_get_writables_path(
    char * recipient,
    const uint32_t recipient_size)
{
    #ifdef COMMON_IGNORE_ASSERTS
    (void)recipient_size;
    #endif
    
    NSArray * paths = NSSearchPathForDirectoriesInDomains(
        NSApplicationSupportDirectory,
        NSUserDomainMask,
        YES);
    
    NSString * libraryDirectory = [paths objectAtIndex:0];
    
    char * library_dir =
        (char *)[libraryDirectory
            cStringUsingEncoding: NSUTF8StringEncoding];
    
    #ifndef LOGGER_IGNORE_ASSERTS
    uint32_t len = common_get_string_length(library_dir);
    log_assert(len < recipient_size);
    #endif
    
    char sep[32];
    platform_get_directory_separator(sep);
    
    common_strcpy_capped(recipient, recipient_size, library_dir);
    common_strcat_capped(recipient, recipient_size, sep);
    common_strcat_capped(recipient, recipient_size, APPLICATION_NAME);
    
    platform_mkdir_if_not_exist(recipient);
}

void * platform_malloc_unaligned_block(
    const uint64_t size)
{
    void * return_value = malloc(size);
    
    return return_value;
}

void platform_enter_fullscreen(void) {
    log_append("Won't move to fullscreen on iOS...\n");
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
    return (engine_globals->window_height - y);
}

void platform_open_folder_in_window_if_possible(
    const char * folderpath)
{
    (void)folderpath;
    
    log_append("Ignoring folder open request - impossible on iOS\n");
}
