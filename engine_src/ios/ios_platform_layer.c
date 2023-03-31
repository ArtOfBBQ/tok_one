#define PLATFORM_IOS

#include "../shared/platform_layer.h"

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
