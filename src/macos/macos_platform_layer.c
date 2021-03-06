#define PLATFORM_NS_FILEMANAGER

#include "../shared/platform_layer.h"
#import "Appkit/Appkit.h"

/*
these variables may not exist on platforms where window resizing is
impossible
*/
extern float current_window_height;
extern float current_window_width;

char * platform_get_cwd() {
    NSString * cwd = [[NSFileManager defaultManager] currentDirectoryPath];
    
    char * return_value = (char *)[cwd UTF8String];
    
    return return_value;
}

float platform_get_current_window_height() {
    // NSScreen *screen = [[NSScreen screens] objectAtIndex:0];
    // NSRect full_screen_rect = [screen frame]; 
    // return (float)NSHeight(full_screen_rect);
    return current_window_height;
}

float platform_get_current_window_width() {
    // NSScreen *screen = [[NSScreen screens] objectAtIndex:0];
    // NSRect full_screen_rect = [screen frame];
    // return (float)NSWidth(full_screen_rect);
    return current_window_width;
}

float platform_x_to_x(const float x) {
    return x;
}

float platform_y_to_y(const float y) {
    return y;
}
