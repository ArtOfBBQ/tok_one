#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include "inttypes.h"

#include "common.h"
#include "window_size.h"
#include "platform_layer.h"
#include "vertex_types.h"
#include "draw_triangle.h"
#include "zpolygon.h"
#include "bitmap_renderer.h"
#include "clientlogic.h"

local_only uint32_t renderer_initialized = false;


void init_renderer(void);
void free_renderer(void);

void software_render(
    Vertex * next_gpu_workload,
    uint32_t * next_gpu_workload_size);

void rotate_triangle(
    Vertex to_rotate[3],
    const float angle);

void translate_triangle(
    Vertex to_translate[3],
    const float x,
    const float y,
    const float z);

#endif

