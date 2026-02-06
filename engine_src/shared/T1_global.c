#include "T1_global.h"

T1Globals * T1_global = NULL;

void T1_global_init(void) {
    
    if (
        T1_global->window_height < 50.0f ||
        T1_global->window_width < 50.0f)
    {
        return;
    }
    
    T1_global->draw_imputed_normals   = false;
    T1_global->draw_fps               = false;
    T1_global->draw_top_touchable_id  = false;
    T1_global->draw_triangles         =  true;
    T1_global->show_profiler          = false;
    T1_global->pause_profiler         = false;
    T1_global->block_mouse            = false;
    
    T1_global->pixelation_div = 1;
    
    T1_global->timedelta_mult = 1.0f;
    
    T1_global->postproc_consts.timestamp = 0;
    T1_global->postproc_consts.blur_pct = 0.18f;
    T1_global->postproc_consts.nonblur_pct = 1.0f;
    T1_global->postproc_consts.color_quantization = 1.0f;
    T1_global->postproc_consts.rgb_add[0] = 0.0f;
    T1_global->postproc_consts.rgb_add[1] = 0.0f;
    T1_global->postproc_consts.rgb_add[2] = 0.0f;
    
    #if T1_FOG_ACTIVE == T1_ACTIVE
    T1_global->postproc_consts.fog_color[0] = 1.0f;
    T1_global->postproc_consts.fog_color[1] = 1.0f;
    T1_global->postproc_consts.fog_color[2] = 1.0f;
    T1_global->postproc_consts.fog_factor = 0.0f;
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_global->last_clickray_origin[0]    = 0.0f;
    T1_global->last_clickray_origin[1]    = 0.0f;
    T1_global->last_clickray_origin[2]    = 0.0f;
    T1_global->last_clickray_direction[0] = 0.0f;
    T1_global->last_clickray_direction[1] = 0.0f;
    T1_global->last_clickray_direction[2] = 1.0f;
}

void T1_global_update_window_pos(
    float left,
    float bottom)
{
    T1_global->window_left = left;
    T1_global->window_bottom = bottom;
}

void T1_global_update_window_size(
    float width,
    float height,
    uint64_t at_timestamp_us)
{
    T1_global->window_height = height;
    T1_global->window_width = width;
    
    T1_global->last_resize_request_us = at_timestamp_us;
}

float T1_global_get_z_mul_for_depth(
    const int32_t for_mesh_id,
    const float for_depth)
{
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    log_assert(for_mesh_id >= 0);
    log_assert(for_mesh_id < (int32_t)T1_mesh_summary_list_size);
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    float return_value =
        for_depth / T1_mesh_summary_list[for_mesh_id].
            base_depth;
    
    return return_value;
}

float T1_global_get_y_mul_for_height(
    const int32_t for_mesh_id,
    const float for_height)
{
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    log_assert(for_mesh_id >= 0);
    log_assert(for_mesh_id < (int32_t)T1_mesh_summary_list_size);
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    float return_value =
        for_height / T1_mesh_summary_list[for_mesh_id].
            base_height;
    
    return return_value;
}

float T1_global_get_x_mul_for_width(
    const int32_t for_mesh_id,
    const float for_width)
{
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    log_assert(for_mesh_id >= 0);
    log_assert(for_mesh_id < (int32_t)T1_mesh_summary_list_size);
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    float return_value =
        for_width /
            T1_mesh_summary_list[for_mesh_id].
                base_width;
    
    return return_value;
}
