#ifndef WINDOW_SIZE_H
#define WINDOW_SIZE_H

#include "common.h"
#include "cpu_gpu_shared_types.h"

extern bool32_t visual_debug_mode;
extern int32_t visual_debug_highlight_touchable_id;
extern float visual_debug_ray_origin_direction[9];
extern float visual_debug_collision[3];
extern float visual_debug_collision_size;

extern float window_height;
extern float window_width;
extern float aspect_ratio;

extern uint64_t last_resize_request_at;
extern bool32_t request_post_resize_clearscreen;

float screenspace_x_to_x(const float screenspace_x);
float screenspace_y_to_y(const float screenspace_y);
float screenspace_height_to_height(const float screenspace_height);
float screenspace_width_to_width(const float screenspace_width);

extern GPU_ProjectionConstants projection_constants;

void init_projection_constants(void);

#endif
