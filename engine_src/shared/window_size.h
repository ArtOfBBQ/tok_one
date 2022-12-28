#ifndef WINDOW_SIZE_H
#define WINDOW_SIZE_H

#include "common.h"

extern float window_height;
extern float window_width;
extern float aspect_ratio;

extern uint64_t last_resize_request_at;
extern bool32_t request_post_resize_clearscreen;

float screenspace_x_to_x(const float screenspace_x);
float screenspace_y_to_y(const float screenspace_y);
float screenspace_height_to_height(const float screenspace_height);
float screenspace_width_to_width(const float screenspace_width);

// projection constants
typedef struct ProjectionConstants {
    float near;
    float far;
    float field_of_view_rad;
    float field_of_view_modifier;
} ProjectionConstants;

extern ProjectionConstants projection_constants;

void init_projection_constants(void);

#endif
