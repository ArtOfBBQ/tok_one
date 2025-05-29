#ifndef TOKONE_PROFILER_H
#define TOKONE_PROFILER_H

#include <stdint.h>

#include "T1_common.h"
#include "T1_logger.h"
#include "T1_zspriteid.h"
#include "T1_userinput.h"

#include "T1_engine_globals.h"
#include "T1_zsprite.h"
#include "T1_text.h"


#if PROFILER_ACTIVE

// #include <>

void profiler_init(
    const uint64_t clock_frequency,
    void * (* profiler_malloc_function)(size_t));

void profiler_new_frame(void);

void profiler_start(const char * function_name);

void profiler_end(const char * function_name);

/*
GUI functions
*/
void profiler_handle_touches(void);

void profiler_draw_labels(void);

#endif // PROFILER_ACTIVE

#endif // TOKONE_PROFILER_H
