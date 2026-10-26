#include "T1_global.h"

#include "T1_assert.h"
#include "T1_mesh_summary.h" // TODO: encapsulate me

T1Globals * T1_global = NULL;

void T1_global_init(void) {
    if (
        T1_global->window_wh[0] < 50.0f ||
        T1_global->window_wh[1] < 50.0f)
    {
        return;
    }
    
    T1_global->draw_imputed_normals   = 0;
    T1_global->draw_fps               = 0;
    T1_global->draw_triangles         = 1;
    T1_global->block_mouse            = 0;
        
    T1_global->timedelta_mult = 1.00f;
    
    T1_global->frame_consts.timestamp = 0;
}

void T1_global_update_window_pos(
    f32 left,
    f32 bottom)
{
    T1_global->window_left = left;
    T1_global->window_bottom = bottom;
}

void T1_global_update_window_size(
    f32 width,
    f32 height,
    u64 at_timestamp_us)
{
    T1_global->window_wh[0] = width;
    T1_global->window_wh[1] = height;
    
    T1_global->last_resize_request_us = at_timestamp_us;
}

f32 T1_global_get_z_mul_for_depth(
    s32 for_mesh_id,
    f32 for_depth)
{
    T1_assert(for_mesh_id >= 0);
    T1_assert(for_mesh_id < (s32)T1_mesh_summary_list_size);
    
    f32 return_value =
        for_depth / T1_mesh_summary_list[for_mesh_id].
            base_depth;
    
    return return_value;
}

f32 T1_global_get_y_mul_for_height(
    s32 for_mesh_id,
    f32 for_height)
{
    #if T1_ASSERTS_ACTIVE == T1_ACTIVE
    T1_assert(for_mesh_id >= 0);
    T1_assert(for_mesh_id < (s32)T1_mesh_summary_list_size);
    #elif T1_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    f32 return_value =
        for_height / T1_mesh_summary_list[for_mesh_id].
            base_height;
    
    return return_value;
}

f32 T1_global_get_x_mul_for_width(
    s32 for_mesh_id,
    f32 for_width)
{
    T1_assert(for_mesh_id >= 0);
    T1_assert(for_mesh_id < (s32)T1_mesh_summary_list_size);
    
    f32 return_value =
        for_width /
            T1_mesh_summary_list[for_mesh_id].
                base_width;
    
    return return_value;
}
