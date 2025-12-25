#ifndef T1_RENDER_VIEW_H
#define T1_RENDER_VIEW_H

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_cpu_gpu_shared_types.h"
#include "T1_logger.h"

extern T1GPURenderView * T1_render_views;
extern T1GPURenderView * T1_camera; // convenience
extern uint32_t T1_render_views_size;

void T1_render_view_init(void);

void T1_render_view_validate(void);

#endif // T1_RENDER_VIEW_H
