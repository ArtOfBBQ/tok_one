#include "../shared/platform_layer.h"
#import <UIKit/UIKit.h>

float platform_get_current_window_height() {
    return [UIScreen mainScreen].bounds.size.height;
}

float platform_get_current_window_width() {
    return [UIScreen mainScreen].bounds.size.width;
}

float platform_get_current_window_left() {
    return 0.0f;
}

float platform_get_current_window_bottom() {
    return 0.0f;
}

float platform_x_to_x(const float x) {
    return x;
}

// 5 should become about 635,
// 635 should become about 5
float platform_y_to_y(const float y) {
    return (window_height - y);
}
