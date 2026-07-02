#ifndef T1_PROFILER_H
#define T1_PROFILER_H

#include "T1_stdint.h"

#if T1_PROFILER_ACTIVE == T1_ACTIVE

void T1_profiler_init(
    const u64 clock_frequency,
    void * (* profiler_malloc_function)(u64));

void T1_profiler_new_frame(void);

void T1_profiler_start(const c8 * func_name);
void T1_profiler_end(const c8 * func_name);

/*
GUI functions
*/
// returns true if touch was overridden
b8 T1_profiler_handle_lclick(
    const s32 touch_id);
void T1_profiler_handle_touches(void);

void T1_profiler_draw_labels(void);
#elif T1_PROFILER_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_PROFILER_ACTIVE undefined"
#endif // T1_PROFILER_ACTIVE

#endif // T1_PROFILER_H
