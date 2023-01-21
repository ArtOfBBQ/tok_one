#ifndef WINDOW_SIZE_H
#define WINDOW_SIZE_H

#include "common.h"
#include "cpu_gpu_shared_types.h"

typedef struct WindowGlobals {
    bool32_t visual_debug_mode;
    int32_t visual_debug_highlight_touchable_id;
    float visual_debug_ray_origin_direction[9];
    float visual_debug_collision[3];
    float visual_debug_collision_size;
    
    float window_height;
    float window_width;
    float aspect_ratio;
    
    uint64_t last_resize_request_at;
    bool32_t request_post_resize_clearscreen;
    GPU_ProjectionConstants projection_constants;
} WindowGlobals;

extern WindowGlobals * window_globals;

float screenspace_x_to_x(const float screenspace_x, const float given_z);
float screenspace_y_to_y(const float screenspace_y, const float given_z);
float screenspace_height_to_height(const float screenspace_height);
float screenspace_width_to_width(const float screenspace_width);

void init_projection_constants(void);

#endif
