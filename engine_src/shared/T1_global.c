#include "T1_global.h"

#include "T1_assert.h"

T1Globals * T1_global = NULL;

void T1_global_init(void) {
    if (
        T1_global->window_wh[0] < 50.0f ||
        T1_global->window_wh[1] < 50.0f)
    {
        return;
    }
    
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
