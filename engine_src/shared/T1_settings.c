#include "T1_settings.h"

#include "T1_global.h"

PerfSettings * T1_perf_settings = NULL;

void T1_settings_init(
    void * arg_malloc_func(u64))
{
    T1_perf_settings = (PerfSettings *)
    arg_malloc_func(sizeof(PerfSettings));
    
    T1_std_memset(
        T1_perf_settings,
        0,
        sizeof(PerfSettings));
    
    T1_perf_settings->render_width_max  = 640;
    T1_perf_settings->skip_background_shading = false;
}

static float T1_settings_get_render_mult(void) {
    return 
        (float)T1_perf_settings->render_width_max /
        (float)T1_global->window_wh[0];
}

uint32_t T1_settings_get_render_width(void) {    
    float mult = T1_settings_get_render_mult();
    
    return (uint32_t)(
        (float)T1_global->window_wh[0] * mult);
}

uint32_t T1_settings_get_render_height(void) {    
    float mult = T1_settings_get_render_mult();
    
    return (uint32_t)(
        (float)T1_global->window_wh[1] * mult);
}
