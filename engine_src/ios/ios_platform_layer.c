#define PLATFORM_IOS

#include "../shared/platform_layer.h"

float platform_get_current_window_height(void) {
    return (float)[UIScreen mainScreen].bounds.size.height;
}

float platform_get_current_window_width(void) {
    return (float)[UIScreen mainScreen].bounds.size.width;
}

float platform_get_current_window_left(void) {
    return 0.0f;
}

float platform_get_current_window_bottom(void) {
    return 0.0f;
}

float platform_x_to_x(const float x) {
    return x;
}

// 5 should become about 635,
// 635 should become about 5
float platform_y_to_y(const float y) {
    return (window_globals->window_height - y);
}
