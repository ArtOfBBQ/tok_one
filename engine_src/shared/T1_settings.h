#ifndef T1_SETTINGS_H
#define T1_SETTINGS_H

#include "T1_stdint.h"

typedef struct PerfSettings {
    u32 render_width_max;
    u8 skip_background_shading;
} PerfSettings;

extern PerfSettings * T1_perf_settings;

void T1_settings_init(void * arg_malloc_func(u64));

u32 T1_settings_get_render_width(void);
u32 T1_settings_get_render_height(void);

#endif // T1_SETTINGS_H

