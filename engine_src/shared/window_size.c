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

void init_projection_constants() {
    
    if (
        window_globals->window_height < 50.0f ||
        window_globals->window_width < 50.0f)
    {
        return;
    }
    
    GPU_ProjectionConstants * pjc = &window_globals->projection_constants;
    
    pjc->near = 0.1f;
    pjc->far = 10.0f;
    
    float field_of_view = 90.0f;
    pjc->field_of_view_rad = ((field_of_view * 0.5f) / 180.0f) * 3.14159f;
    pjc->field_of_view_modifier = 1.0f / tanf(pjc->field_of_view_rad);
    pjc->q = pjc->far / (pjc->far - pjc->near);
    pjc->x_multiplier =
        window_globals->aspect_ratio * pjc->field_of_view_modifier;
    pjc->y_multiplier = pjc->field_of_view_modifier;
    
    window_globals->visual_debug_collision[0] =  0.0f;
    window_globals->visual_debug_collision[1] =  0.0f;
    window_globals->visual_debug_collision[2] = -5.0f;
}
