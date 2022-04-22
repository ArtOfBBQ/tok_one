#ifndef BITMAP_RENDERER_H
#define BITMAP_RENDERER_H

/*
Directly adjust pixels in a bitmap image, then copy that image
to gpu memory and display it to the screen as is.

For now I just want this to draw a little 2D minimap in
the corner of the screen to help me understand what's going on
with my 3D objects

You could delete all of the 3d graphics code and use this to
draw a bitmap over the entire screen and update it every frame,
so you can enjoy retro game programming from when gpu's didn't
exist.
*/

#include <math.h>

#include "common.h"
#include "window_size.h"
#include "texquad_type.h"
#include "debigulator/src/decodedimage.h"
#include "vertex_types.h"
#include "draw_triangle.h"

#define TEXQUADS_TO_RENDER_ARRAYSIZE 2000
extern TexQuad texquads_to_render[TEXQUADS_TO_RENDER_ARRAYSIZE];
extern uint32_t texquads_to_render_size;

bool32_t touchable_id_to_texquad_object_id(
    const int32_t touchable_id,
    uint32_t * object_id_out);

void request_texquad_renderable(
    TexQuad * to_add);

void move_texquad_object(
    uint32_t with_object_id,
    float delta_x,
    float delta_y);

void delete_texquad_object(
    uint32_t with_object_id);

/* Draw bitmap(s) of pixels and add them to the gpu's workload */
void draw_texquads_to_render(
    Vertex * next_gpu_workload,
    uint32_t * next_gpu_workload_size);

#endif

