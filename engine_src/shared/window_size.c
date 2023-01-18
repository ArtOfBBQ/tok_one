#include "window_size.h"

GPU_ProjectionConstants projection_constants;

bool32_t visual_debug_mode = false;
int32_t visual_debug_highlight_touchable_id = -1;
float visual_debug_ray_origin_direction[9];
float visual_debug_collision[3];
float visual_debug_collision_size = 0.01f;

float window_height = 0.0f;
float window_width  = 0.0f;
float aspect_ratio  = 1.0f;

uint64_t last_resize_request_at = 99999999999;
bool32_t request_post_resize_clearscreen = false;

float screenspace_x_to_x(const float screenspace_x, const float given_z)
{
    return (((screenspace_x * 2.0f) / window_width) - 1.0f) * given_z;
}

float screenspace_y_to_y(const float screenspace_y, const float given_z)
{
    return (((screenspace_y * 2.0f) / window_height) - 1.0f) * given_z;
}

float screenspace_height_to_height(const float screenspace_height)
{
    return (screenspace_height * 2.0f) / window_height;
}

float screenspace_width_to_width(const float screenspace_width)
{
    return (screenspace_width * 2.0f) / window_width;
}

void init_projection_constants() {
    
    if (window_height < 50.0f || window_width < 50.0f) {
        return;
    }
    
    GPU_ProjectionConstants * pjc = &projection_constants;
    
    pjc->near = 0.1f;
    pjc->far = 20.0f;
    
    float field_of_view = 90.0f;
    pjc->field_of_view_rad = ((field_of_view * 0.5f) / 180.0f) * 3.14159f;
    pjc->field_of_view_modifier = 1.0f / tok_tanf(pjc->field_of_view_rad); 
    pjc->q = pjc->far / (pjc->far - pjc->near);
    pjc->x_multiplier = aspect_ratio * pjc->field_of_view_modifier;
    pjc->y_multiplier = pjc->field_of_view_modifier;
    
    visual_debug_collision[0] =  0.0f;
    visual_debug_collision[1] =  0.0f;
    visual_debug_collision[2] = -5.0f;
}
