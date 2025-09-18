#include "T1_engine_globals.h"

EngineGlobals * engine_globals = NULL;

float engineglobals_x_to_screenspace_x(
    const float x,
    const float given_z)
{
    return (x * engine_globals->project_consts.x_multiplier + given_z) * engine_globals->window_width / (2.0f * given_z);
}

float engineglobals_screenspace_x_to_x(
    const float screenspace_x,
    const float given_z)
{
    return (
        (((screenspace_x * 2.0f) / engine_globals->window_width) - 1.0f)
            * given_z)
            / engine_globals->project_consts.x_multiplier;
}

float engineglobals_y_to_screenspace_y(
    const float y,
    const float given_z)
{
    return (
        y *
        engine_globals->project_consts.field_of_view_modifier + given_z) * engine_globals->window_height / (2.0f * given_z);
}

float engineglobals_screenspace_y_to_y(
    const float screenspace_y,
    const float given_z)
{
    return (
        (((screenspace_y * 2.0f) / engine_globals->window_height) - 1.0f)
            * given_z)
                / engine_globals->project_consts.field_of_view_modifier;
}

float engineglobals_screenspace_height_to_height(
    const float screenspace_height,
    const float given_z)
{
    return ((
        (screenspace_height * 2.0f) / engine_globals->window_height)
            * given_z)
                / engine_globals->project_consts.field_of_view_modifier;
}

float engineglobals_screenspace_width_to_width(
    const float screenspace_width,
    const float given_z)
{
    return
        (((screenspace_width * 2.0f) / engine_globals->window_width)
            * given_z)
            / engine_globals->project_consts.x_multiplier;
}

void engineglobals_init(void) {
    
    if (
        engine_globals->window_height < 50.0f ||
        engine_globals->window_width < 50.0f)
    {
        return;
    }
    
    T1GPUProjectConsts * pjc = &engine_globals->project_consts;
    
    pjc->znear =  0.1f;
    pjc->zfar  =  6.0f;
    
    float field_of_view = 75.0f;
    pjc->field_of_view_rad = ((field_of_view * 0.5f) / 180.0f) * 3.14159f;
    
    pjc->field_of_view_modifier = 1.0f / tanf(pjc->field_of_view_rad);
    
    // pjc->q = pjc->zfar / (pjc->zfar - pjc->znear);
    pjc->x_multiplier = engine_globals->aspect_ratio *
        pjc->field_of_view_modifier;
    pjc->y_multiplier = pjc->field_of_view_modifier;
    
    engine_globals->draw_imputed_normals   = false;
    engine_globals->draw_fps               = false;
    engine_globals->draw_top_touchable_id  = false;
    engine_globals->draw_triangles         =  true;
    engine_globals->show_profiler          = false;
    engine_globals->pause_profiler         = false;
    engine_globals->block_mouse            = false;
    
    engine_globals->pixelation_div = 1;
    
    engine_globals->timedelta_mult = 1.0f;
    
    engine_globals->postproc_consts.timestamp = 0;
    engine_globals->postproc_consts.blur_pct = 0.18f;
    engine_globals->postproc_consts.nonblur_pct = 1.0f;
    engine_globals->postproc_consts.screen_height =
        engine_globals->window_height;
    engine_globals->postproc_consts.screen_width =
        engine_globals->window_width;
    engine_globals->postproc_consts.color_quantization = 1.0f;
    engine_globals->postproc_consts.rgb_add[0] = 0.0f;
    engine_globals->postproc_consts.rgb_add[1] = 0.0f;
    engine_globals->postproc_consts.rgb_add[2] = 0.0f;
    
    #if T1_FOG_ACTIVE == T1_ACTIVE
    engine_globals->postproc_consts.fog_color[0] = 1.0f;
    engine_globals->postproc_consts.fog_color[1] = 1.0f;
    engine_globals->postproc_consts.fog_color[2] = 1.0f;
    engine_globals->postproc_consts.fog_factor = 0.0f;
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
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
    uint64_t at_timestamp_us)
{
    engine_globals->window_height = height;
    engine_globals->window_width = width;
    
    engine_globals->aspect_ratio = height / width;
    
    engine_globals->last_resize_request_us = at_timestamp_us;
}
