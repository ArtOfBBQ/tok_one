#include "T1_engineglobals.h"

T1EngineGlobals * T1_engine_globals = NULL;

float T1_engineglobals_x_to_screenspace_x(
    const float x,
    const float given_z)
{
    return (x * T1_engine_globals->project_consts.x_multiplier + given_z) * T1_engine_globals->window_width / (2.0f * given_z);
}

float T1_engineglobals_screenspace_x_to_x(
    const float screenspace_x,
    const float given_z)
{
    return (
        (((screenspace_x * 2.0f) / T1_engine_globals->window_width) - 1.0f)
            * given_z)
            / T1_engine_globals->project_consts.x_multiplier;
}

float T1_engineglobals_y_to_screenspace_y(
    const float y,
    const float given_z)
{
    return (
        y *
        T1_engine_globals->project_consts.field_of_view_modifier + given_z) * T1_engine_globals->window_height / (2.0f * given_z);
}

float T1_engineglobals_screenspace_y_to_y(
    const float screenspace_y,
    const float given_z)
{
    return (
        (((screenspace_y * 2.0f) / T1_engine_globals->window_height) - 1.0f)
            * given_z)
                / T1_engine_globals->project_consts.field_of_view_modifier;
}

float T1_engineglobals_screenspace_height_to_height(
    const float screenspace_height,
    const float given_z)
{
    return ((
        (screenspace_height * 2.0f) / T1_engine_globals->window_height)
            * given_z)
                / T1_engine_globals->project_consts.field_of_view_modifier;
}

float T1_engineglobals_screenspace_width_to_width(
    const float screenspace_width,
    const float given_z)
{
    return
        (((screenspace_width * 2.0f) / T1_engine_globals->window_width)
            * given_z)
            / T1_engine_globals->project_consts.x_multiplier;
}

void T1_engineglobals_init(void) {
    
    if (
        T1_engine_globals->window_height < 50.0f ||
        T1_engine_globals->window_width < 50.0f)
    {
        return;
    }
    
    T1GPUProjectConsts * pjc = &T1_engine_globals->project_consts;
    
    pjc->znear =  0.1f;
    pjc->zfar  =  6.0f;
    
    float field_of_view = 75.0f;
    pjc->field_of_view_rad = ((field_of_view * 0.5f) / 180.0f) * 3.14159f;
    
    pjc->field_of_view_modifier = 1.0f / tanf(pjc->field_of_view_rad);
    
    // pjc->q = pjc->zfar / (pjc->zfar - pjc->znear);
    pjc->x_multiplier = T1_engine_globals->aspect_ratio *
        pjc->field_of_view_modifier;
    pjc->y_multiplier = pjc->field_of_view_modifier;
    
    T1_engine_globals->draw_imputed_normals   = false;
    T1_engine_globals->draw_fps               = false;
    T1_engine_globals->draw_top_touchable_id  = false;
    T1_engine_globals->draw_triangles         =  true;
    T1_engine_globals->show_profiler          = false;
    T1_engine_globals->pause_profiler         = false;
    T1_engine_globals->block_mouse            = false;
    
    T1_engine_globals->pixelation_div = 1;
    
    T1_engine_globals->timedelta_mult = 1.0f;
    
    T1_engine_globals->postproc_consts.timestamp = 0;
    T1_engine_globals->postproc_consts.blur_pct = 0.18f;
    T1_engine_globals->postproc_consts.nonblur_pct = 1.0f;
    T1_engine_globals->postproc_consts.screen_height =
        T1_engine_globals->window_height;
    T1_engine_globals->postproc_consts.screen_width =
        T1_engine_globals->window_width;
    T1_engine_globals->postproc_consts.color_quantization = 1.0f;
    T1_engine_globals->postproc_consts.rgb_add[0] = 0.0f;
    T1_engine_globals->postproc_consts.rgb_add[1] = 0.0f;
    T1_engine_globals->postproc_consts.rgb_add[2] = 0.0f;
    
    #if T1_FOG_ACTIVE == T1_ACTIVE
    T1_engine_globals->postproc_consts.fog_color[0] = 1.0f;
    T1_engine_globals->postproc_consts.fog_color[1] = 1.0f;
    T1_engine_globals->postproc_consts.fog_color[2] = 1.0f;
    T1_engine_globals->postproc_consts.fog_factor = 0.0f;
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_engine_globals->last_clickray_origin[0]    = 0.0f;
    T1_engine_globals->last_clickray_origin[1]    = 0.0f;
    T1_engine_globals->last_clickray_origin[2]    = 0.0f;
    T1_engine_globals->last_clickray_direction[0] = 0.0f;
    T1_engine_globals->last_clickray_direction[1] = 0.0f;
    T1_engine_globals->last_clickray_direction[2] = 1.0f;
}

void T1_engineglobals_update_window_position(
    float left,
    float bottom)
{
    T1_engine_globals->window_left = left;
    T1_engine_globals->window_bottom = bottom;
}

void T1_engineglobals_update_window_size(
    float width,
    float height,
    uint64_t at_timestamp_us)
{
    T1_engine_globals->window_height = height;
    T1_engine_globals->window_width = width;
    
    T1_engine_globals->aspect_ratio = height / width;
    
    T1_engine_globals->last_resize_request_us = at_timestamp_us;
}
