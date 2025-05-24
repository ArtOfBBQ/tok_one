#include "window_size.h"

WindowGlobals * window_globals = NULL;


float windowsize_screenspace_x_to_x(
    const float screenspace_x,
    const float given_z)
{
    return (
        (((screenspace_x * 2.0f) / window_globals->window_width) - 1.0f)
            * given_z)
            / window_globals->projection_constants.x_multiplier;
}

float windowsize_screenspace_y_to_y(
    const float screenspace_y,
    const float given_z)
{
    return (
        (((screenspace_y * 2.0f) / window_globals->window_height) - 1.0f)
            * given_z)
                / window_globals->projection_constants.field_of_view_modifier;
}

float windowsize_screenspace_height_to_height(
    const float screenspace_height,
    const float given_z)
{
    return ((
        (screenspace_height * 2.0f) / window_globals->window_height)
            * given_z)
                / window_globals->projection_constants.field_of_view_modifier;
}

float windowsize_screenspace_width_to_width(
    const float screenspace_width,
    const float given_z)
{
    return
        (((screenspace_width * 2.0f) / window_globals->window_width)
            * given_z)
            / window_globals->projection_constants.x_multiplier;
}

void windowsize_init(void) {
    
    if (
        window_globals->window_height < 50.0f ||
        window_globals->window_width < 50.0f)
    {
        return;
    }
    
    GPUProjectionConstants * pjc = &window_globals->projection_constants;
    
    pjc->znear =  0.03f;
    pjc->zfar  =  25.0f;
    
    float field_of_view = 75.0f;
    pjc->field_of_view_rad = ((field_of_view * 0.5f) / 180.0f) * 3.14159f;
    
    pjc->field_of_view_modifier = 1.0f / tanf(pjc->field_of_view_rad);
    
    // pjc->q = pjc->zfar / (pjc->zfar - pjc->znear);
    pjc->x_multiplier = window_globals->aspect_ratio *
        pjc->field_of_view_modifier;
    pjc->y_multiplier = pjc->field_of_view_modifier;
    
    window_globals->draw_clickray         = false;
    window_globals->draw_imputed_normals  = false;
    window_globals->draw_fps              = false;
    window_globals->draw_top_touchable_id = false;
    window_globals->draw_triangles        =  true;
    window_globals->show_profiler         = false;
    window_globals->pause_profiler        = false;
    window_globals->block_mouse           = false;
    
    window_globals->pixelation_div = 2;
    
    window_globals->postprocessing_constants.timestamp = 0;
    window_globals->postprocessing_constants.blur_pct = 0.18f;
    window_globals->postprocessing_constants.nonblur_pct = 1.0f;
    window_globals->postprocessing_constants.screen_height =
        window_globals->window_height;
    window_globals->postprocessing_constants.screen_width =
        window_globals->window_width;
    window_globals->postprocessing_constants.color_quantization = 1.0f;
    window_globals->postprocessing_constants.rgb_add[0] = 0.0f;
    window_globals->postprocessing_constants.rgb_add[1] = 0.0f;
    window_globals->postprocessing_constants.rgb_add[2] = 0.0f;
    
    window_globals->postprocessing_constants.fog_color[0] = 0.25f;
    window_globals->postprocessing_constants.fog_color[1] = 0.45f;
    window_globals->postprocessing_constants.fog_color[2] = 0.25f;
    window_globals->postprocessing_constants.fog_factor = 0.35f;
    
    window_globals->last_clickray_origin[0]    = 0.0f;
    window_globals->last_clickray_origin[1]    = 0.0f;
    window_globals->last_clickray_origin[2]    = 0.0f;
    window_globals->last_clickray_direction[0] = 0.0f;
    window_globals->last_clickray_direction[1] = 0.0f;
    window_globals->last_clickray_direction[2] = 1.0f;
}

void windowsize_update_window_position(
    float left,
    float bottom)
{
    window_globals->window_left = left;
    window_globals->window_bottom = bottom;
}

void windowsize_update_window_size(
    float width,
    float height,
    uint64_t at_timestamp_microseconds)
{
    window_globals->window_height = height;
    window_globals->window_width = width;
    
    window_globals->aspect_ratio = height / width;
    
    window_globals->last_resize_request_at = at_timestamp_microseconds;
}
