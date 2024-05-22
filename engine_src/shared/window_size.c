#include "window_size.h"

WindowGlobals * window_globals = NULL;


float screenspace_x_to_x(const float screenspace_x, const float given_z)
{
    return (
        ((screenspace_x * 2.0f) / window_globals->window_width) - 1.0f)
            * given_z
            / window_globals->aspect_ratio;
}

float screenspace_y_to_y(const float screenspace_y, const float given_z)
{
    return (
        ((screenspace_y * 2.0f) / window_globals->window_height) - 1.0f)
            * given_z;
}

float screenspace_height_to_height(
    const float screenspace_height,
    const float given_z)
{
    return (
        (screenspace_height * 2.0f) / window_globals->window_height)
            * given_z;
}

float screenspace_width_to_width(
    const float screenspace_width,
    const float given_z)
{
    return
        ((screenspace_width * 2.0f) / window_globals->window_width)
            * given_z
            / window_globals->aspect_ratio;
}

void init_projection_constants(void) {
    
    if (
        window_globals->window_height < 50.0f ||
        window_globals->window_width < 50.0f)
    {
        return;
    }
    
    GPUProjectionConstants * pjc = &window_globals->projection_constants;
    
    pjc->znear = 0.03f;
    pjc->zfar =  8.50f;
    
    // this hardcoded value was calculated with:
    // float field_of_view = 90.0f;
    // pjc->field_of_view_rad = ((field_of_view * 0.5f) / 180.0f) * 3.14159f;
    pjc->field_of_view_rad = 0.7853975f;
    
    // this hardcoded value was calculated with:
    // pjc->field_of_view_modifier = 1.0f / tanf(pjc->field_of_view_rad);
    pjc->field_of_view_modifier = 1.000001326795777f;
    
    pjc->q = pjc->zfar / (pjc->zfar - pjc->znear);
    pjc->x_multiplier =
        window_globals->aspect_ratio * pjc->field_of_view_modifier;
    pjc->y_multiplier = pjc->field_of_view_modifier;
    
    window_globals->visual_debug_highlight_touchable_id = -1;
    window_globals->visual_debug_last_clicked_touchable_id = -1;
    
    window_globals->visual_debug_collision[0] =  0.0f;
    window_globals->visual_debug_collision[1] =  0.0f;
    window_globals->visual_debug_collision[2] = -5.0f;
}

void update_window_position(
    float left,
    float bottom)
{
    window_globals->window_left = left;
    window_globals->window_bottom = bottom;
}

void update_window_size(
    float width,
    float height,
    uint64_t at_timestamp_microseconds)
{
    window_globals->window_height = height;
    window_globals->window_width = width;
    
    window_globals->aspect_ratio = height / width;
    
    window_globals->last_resize_request_at = at_timestamp_microseconds;
}
