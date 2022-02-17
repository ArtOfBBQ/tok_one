#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include "window_size.h"
#include "vertex_types.h"
#include "box.h"

static zPolygon * box;
static zVertex camera = {0.0f, 0.0f, 0.0f};

void renderer_init(void);

void software_render(
    ColoredVertex * next_gpu_workload,
    uint32_t * next_gpu_workload_size);

void draw_triangle(
    ColoredVertex * vertices_recipient,
    uint32_t * vertex_count_recipient,
    ColoredVertex input[3]);

void rotate_triangle(
    ColoredVertex to_rotate[3],
    const float angle);

void translate_triangle(
    ColoredVertex to_translate[3],
    const float x,
    const float y,
    const float z);

#endif

