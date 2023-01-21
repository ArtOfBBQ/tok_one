#define PLATFORM_NS_FILEMANAGER

#include "../shared/platform_layer.h"
#import "Appkit/Appkit.h"

void platform_get_cwd(char * recipient, const uint32_t recipient_size) {
    NSString * cwd = [[NSFileManager defaultManager] currentDirectoryPath];
    
    char * return_value = (char *)[cwd UTF8String];
    
    strcpy_capped(recipient, recipient_size, return_value);
}

float platform_get_current_window_height(void) {
    // NSScreen *screen = [[NSScreen screens] objectAtIndex:0];
    // NSRect full_screen_rect = [screen frame]; 
    // return (float)NSHeight(full_screen_rect);
    if (window_globals->window_height < 1.0f) {
        window_globals->window_height = 800.0f;
    }
    
    return window_globals->window_height;
}

float platform_get_current_window_width(void) {
    // NSScreen *screen = [[NSScreen screens] objectAtIndex:0];
    // NSRect full_screen_rect = [screen frame];
    // return (float)NSWidth(full_screen_rect);
    if (window_globals->window_width < 1.0f) {
        window_globals->window_width = 1000.0f;
    }
    
    return window_globals->window_width;
}

float platform_x_to_x(const float x) {
    return x;
}

float platform_y_to_y(const float y) {
    return y;
}

