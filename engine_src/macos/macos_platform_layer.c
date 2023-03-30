#define PLATFORM_NS_FILEMANAGER

#include "../shared/platform_layer.h"
#import "Appkit/Appkit.h"

void platform_close_application(void) {
    [NSApp terminate: nil];
}

void platform_get_cwd(char * recipient, const uint32_t recipient_size) {
    NSString * cwd = [[NSFileManager defaultManager] currentDirectoryPath];
    
    char * return_value = (char *)[cwd UTF8String];
    
    strcpy_capped(recipient, recipient_size, return_value);
}

float platform_x_to_x(const float x) {
    return x;
}

float platform_y_to_y(const float y) {
    return y;
}

