#ifndef TOKONE_PROFILER_H
#define TOKONE_PROFILER_H

#include <stdint.h>

#include "common.h"
#include "logger.h"
#include "objectid.h"
#include "userinput.h"

#include "window_size.h"
#include "zpolygon.h"
#include "text.h"


#ifdef PROFILER_ACTIVE

void profiler_init(
    void * (* profiler_malloc_function)(size_t));

void profiler_new_frame(void);

void profiler_start(const char * function_name);

void profiler_end(const char * function_name);

/*
GUI functions
*/
void profiler_handle_touches(void);

void profiler_draw_labels(void);

#endif

#endif // TOKONE_PROFILER_H
