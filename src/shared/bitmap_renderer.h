#ifndef BITMAP_RENDERER_H
#define BITMAP_RENDERER_H

#include "platform_layer.h"
#include "decodedimage.h"
#include "vertex_types.h"
#include "draw_triangle.h"

extern DecodedImage bitmap;

void bitmap_renderer_init();
void bitmap_clear();

/* Draw bitmap(s) of pixels and add them to the gpu's workload */
void bitmap_render(
    Vertex * next_gpu_workload,
    uint32_t * next_gpu_workload_size);

#endif

