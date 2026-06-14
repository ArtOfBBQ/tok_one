#ifndef T1_PROFILER_H
#define T1_PROFILER_H

#include <stdint.h>

#if T1_PROFILER_ACTIVE == T1_ACTIVE

void T1_profiler_init(
    const uint64_t clock_frequency,
    void * (* profiler_malloc_function)(size_t));

void T1_profiler_new_frame(void);

void T1_profiler_start(const char * function_name);

void T1_profiler_end(const char * function_name);

/*
GUI functions
*/
void T1_profiler_handle_touches(void);

void T1_profiler_draw_labels(void);
#elif T1_PROFILER_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_PROFILER_ACTIVE undefined"
#endif // T1_PROFILER_ACTIVE

#endif // T1_PROFILER_H
