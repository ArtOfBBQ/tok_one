#include "window_size.h"

float window_height = 0.0f;
float window_width = 0.0f;

uint64_t last_resize_request_at = 99999999999;
bool32_t request_post_resize_clearscreen = false;
