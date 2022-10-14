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
#include <stdlib.h>

#include "window_size.h"
#include "texquad_type.h"
#include "decodedimage.h"
#include "vertex_types.h"
#include "draw_triangle.h"
#include "texture_array.h"
#include "lightsource.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TEXQUADS_TO_RENDER_ARRAYSIZE 2000
extern TexQuad * texquads_to_render;
extern uint32_t texquads_to_render_size;

bool32_t touchable_id_to_texquad_object_id(
    const int32_t touchable_id,
    int32_t * object_id_out);

void request_texquad_renderable(
    TexQuad * to_add);

void delete_texquad_object(
    const int32_t with_object_id);

/* Draw bitmap(s) of pixels and add them to the gpu's workload */
void draw_texquads_to_render(
    Vertex * next_gpu_workload,
    uint32_t * next_gpu_workload_size,
    const zLightSource * zlights_transformed);

void clean_deleted_texquads(void);

#ifdef __cplusplus
}
#endif

#endif // BITMAP_RENDERER_H
