#include "window_size.h"

float window_height = 0.0f;
float window_width = 0.0f;
float aspect_ratio = 1.0f;

uint64_t last_resize_request_at = 99999999999;
bool32_t request_post_resize_clearscreen = false;

float screenspace_x_to_x(const float screenspace_x)
{
    return ((screenspace_x * 2.0f) / window_width) - 1.0f;
}

float screenspace_y_to_y(const float screenspace_y)
{
    return ((screenspace_y * 2.0f) / window_height) - 1.0f;
}

float screenspace_height_to_height(const float screenspace_height)
{
    return (screenspace_height * 2.0f) / window_height;
}

float screenspace_width_to_width(const float screenspace_width)
{
    return (screenspace_width * 2.0f) / window_width;
}
