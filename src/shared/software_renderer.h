#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include "window_size.h"
#include "vertex_types.h"
#include "box.h"

void software_render(
    ColoredVertex * next_gpu_workload,
    uint32_t * next_gpu_workload_size);

void draw_triangle(
    ColoredVertex * vertices_recipient,
    uint32_t * vertex_count_recipient,
    ColoredVertex input[3],
    simd_float2 position);

void rotate_triangle(
    ColoredVertex to_rotate[3],
    const float angle);

#endif

