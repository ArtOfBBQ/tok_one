#ifndef T1_RENDER_VIEW_H
#define T1_RENDER_VIEW_H

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_cpu_gpu_shared_types.h"
#include "T1_logger.h"

typedef struct {
    float    xyz[3];           // 12 bytes
    float    xyz_angle[3];     // 12 bytes
    int32_t  write_array_i;
    int32_t  write_slice_i;
    uint32_t width;
    uint32_t height;
    T1RenderViewWriteType write_type;
    uint8_t  draw_outlines;
    uint8_t  deleted;
} T1CPURenderView;

typedef struct {
    T1GPURenderView gpu[T1_RENDER_VIEW_CAP];
    T1CPURenderView cpu[T1_RENDER_VIEW_CAP];
    uint32_t size;
} T1RenderViewCollection;

extern T1RenderViewCollection * T1_render_views;
extern T1CPURenderView * T1_camera; // convenience

void T1_render_view_init(void);

int32_t T1_render_view_fetch_next(void);

void T1_render_view_delete(const int32_t rv_i);

void T1_render_view_validate(void);

#endif // T1_RENDER_VIEW_H
