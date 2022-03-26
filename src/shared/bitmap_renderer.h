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

#include "software_renderer.h"
#include "zpolygon.h"
#include "platform_layer.h"
#include "decodedimage.h"
#include "vertex_types.h"
#include "draw_triangle.h"

#define BITMAP_PIXELS_WIDTH 100
extern DecodedImage minimap;
extern DecodedImage minimap2;

void bitmap_renderer_init();

void minimaps_clear();

void decodedimg_add_zpolygon(
    DecodedImage * to_modify,
    zPolygon * to_add);

void decodedimg_add_triangle(
    DecodedImage * to_modify,
    zTriangle * to_add);

void decodedimg_add_camera(
    DecodedImage * to_modify,
    zCamera * to_add);

/* Draw bitmap(s) of pixels and add them to the gpu's workload */
void minimaps_blit(
    Vertex * next_gpu_workload,
    uint32_t * next_gpu_workload_size);

#endif

