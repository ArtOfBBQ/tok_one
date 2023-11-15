#ifndef WINDOW_SIZE_H
#define WINDOW_SIZE_H

#define CLEARDEPTH 1.0f

#include "common.h"
#include "cpu_gpu_shared_types.h"

typedef struct WindowGlobals {
    bool32_t wireframe_mode;
    bool32_t visual_debug_mode;
    int32_t visual_debug_last_clicked_touchable_id;
    int32_t visual_debug_highlight_touchable_id;
    float visual_debug_ray_origin_direction[9];
    float visual_debug_collision[3];
    float visual_debug_collision_size;
    
    float window_height;
    float window_width;
    float window_left;
    float window_bottom;
    float aspect_ratio;
    
    uint64_t last_resize_request_at;
    GPUProjectionConstants projection_constants;
} WindowGlobals;

extern WindowGlobals * window_globals;

float screenspace_x_to_x(const float screenspace_x, const float given_z);
float screenspace_y_to_y(const float screenspace_y, const float given_z);
float screenspace_height_to_height(
    const float screenspace_height, const float given_z);
float screenspace_width_to_width(
    const float screenspace_width, const float given_z);

void init_projection_constants(void);

void update_window_position(
    float left,
    float bottom);
void update_window_size(
    float width,
    float height,
    uint64_t at_timestamp_microseconds);

#endif

