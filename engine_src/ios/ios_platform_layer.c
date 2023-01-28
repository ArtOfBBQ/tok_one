#define PLATFORM_IOS

#include "../shared/platform_layer.h"

float platform_x_to_x(const float x) {
    return x;
}

// 5 should become about 635,
// 635 should become about 5
float platform_y_to_y(const float y) {
    return (window_globals->window_height - y);
}
