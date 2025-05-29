#include "T1_engine_globals.h"

EngineGlobals * engine_globals = NULL;


float engineglobals_screenspace_x_to_x(
    const float screenspace_x,
    const float given_z)
{
    return (
        (((screenspace_x * 2.0f) / engine_globals->window_width) - 1.0f)
            * given_z)
            / engine_globals->projection_constants.x_multiplier;
}

float engineglobals_screenspace_y_to_y(
    const float screenspace_y,
    const float given_z)
{
    return (
        (((screenspace_y * 2.0f) / engine_globals->window_height) - 1.0f)
            * given_z)
                / engine_globals->projection_constants.field_of_view_modifier;
}

float engineglobals_screenspace_height_to_height(
    const float screenspace_height,
    const float given_z)
{
    return ((
        (screenspace_height * 2.0f) / engine_globals->window_height)
            * given_z)
                / engine_globals->projection_constants.field_of_view_modifier;
}

float engineglobals_screenspace_width_to_width(
    const float screenspace_width,
    const float given_z)
{
    return
        (((screenspace_width * 2.0f) / engine_globals->window_width)
            * given_z)
            / engine_globals->projection_constants.x_multiplier;
}

void engineglobals_init(void) {
    
    if (
        engine_globals->window_height < 50.0f ||
        engine_globals->window_width < 50.0f)
    {
        return;
    }
    
    GPUProjectionConstants * pjc = &engine_globals->projection_constants;
    
    pjc->znear =  0.03f;
    pjc->zfar  =  25.0f;
    
    float field_of_view = 75.0f;
    pjc->field_of_view_rad = ((field_of_view * 0.5f) / 180.0f) * 3.14159f;
    
    pjc->field_of_view_modifier = 1.0f / tanf(pjc->field_of_view_rad);
    
    // pjc->q = pjc->zfar / (pjc->zfar - pjc->znear);
    pjc->x_multiplier = engine_globals->aspect_ratio *
        pjc->field_of_view_modifier;
    pjc->y_multiplier = pjc->field_of_view_modifier;
    
    engine_globals->draw_clickray         = false;
    engine_globals->draw_imputed_normals  = false;
    engine_globals->draw_fps              = false;
    engine_globals->draw_top_touchable_id = false;
    engine_globals->draw_triangles        =  true;
    engine_globals->show_profiler         = false;
    engine_globals->pause_profiler        = false;
    engine_globals->block_mouse           = false;
    
    engine_globals->pixelation_div = 1;
    
    engine_globals->postprocessing_constants.timestamp = 0;
    engine_globals->postprocessing_constants.blur_pct = 0.18f;
    engine_globals->postprocessing_constants.nonblur_pct = 1.0f;
    engine_globals->postprocessing_constants.screen_height =
        engine_globals->window_height;
    engine_globals->postprocessing_constants.screen_width =
        engine_globals->window_width;
    engine_globals->postprocessing_constants.color_quantization = 1.0f;
    engine_globals->postprocessing_constants.rgb_add[0] = 0.0f;
    engine_globals->postprocessing_constants.rgb_add[1] = 0.0f;
    engine_globals->postprocessing_constants.rgb_add[2] = 0.0f;
    
    engine_globals->postprocessing_constants.fog_color[0] = 0.25f;
    engine_globals->postprocessing_constants.fog_color[1] = 0.45f;
    engine_globals->postprocessing_constants.fog_color[2] = 0.25f;
    engine_globals->postprocessing_constants.fog_factor = 0.35f;
    
    engine_globals->last_clickray_origin[0]    = 0.0f;
    engine_globals->last_clickray_origin[1]    = 0.0f;
    engine_globals->last_clickray_origin[2]    = 0.0f;
    engine_globals->last_clickray_direction[0] = 0.0f;
    engine_globals->last_clickray_direction[1] = 0.0f;
    engine_globals->last_clickray_direction[2] = 1.0f;
}

void engineglobals_update_window_position(
    float left,
    float bottom)
{
    engine_globals->window_left = left;
    engine_globals->window_bottom = bottom;
}

void engineglobals_update_window_size(
    float width,
    float height,
    uint64_t at_timestamp_microseconds)
{
    engine_globals->window_height = height;
    engine_globals->window_width = width;
    
    engine_globals->aspect_ratio = height / width;
    
    engine_globals->last_resize_request_at = at_timestamp_microseconds;
}
