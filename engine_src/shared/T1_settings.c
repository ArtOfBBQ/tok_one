#include "T1_settings.h"
#include "T1_global.h"

PerfSettings * T1_perf_settings = NULL;

void T1_settings_init(void * arg_malloc_func(u64))
{
    T1_perf_settings = (PerfSettings *)
    arg_malloc_func(sizeof(PerfSettings));
    
    T1_std_memset(
        T1_perf_settings,
        0,
        sizeof(PerfSettings));
    
    T1_perf_settings->render_width_max  = 2160;
    T1_perf_settings->skip_background_shading = false;
}

static float T1_settings_get_render_mult(void) {
    return 
        (float)T1_perf_settings->render_width_max /
        (float)T1_global->window_wh[0];
}

u32 T1_settings_get_render_width(void) {
    if (T1_global->window_wh[0] <= T1_perf_settings->render_width_max)
    {
        return (u32)T1_global->window_wh[0];
    }
    
    float mult = T1_settings_get_render_mult();
    
    u32 out = (u32)((float)T1_global->window_wh[0] * mult);
    
    while (
        out > (u32)T1_global->window_wh[0] &&
        out % (u32)T1_global->window_wh[0] != 0) {
        out--;
    }
    
    return out;
}

uint32_t T1_settings_get_render_height(void) {
    if (T1_global->window_wh[0] <= T1_perf_settings->render_width_max)
    {
        return (u32)T1_global->window_wh[1];
    }
    
    float mult = T1_settings_get_render_mult();
    
    return (uint32_t)(
        (float)T1_global->window_wh[1] * mult);
}
