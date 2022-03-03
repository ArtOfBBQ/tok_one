#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include "inttypes.h"

#include "window_size.h"
#include "vertex_types.h"
#include "zpolygon.h"
#include "static_redefinitions.h"
#include "decode_png.h"

local_only uint32_t renderer_initialized = false;
local_only zPolygon * zpolygons_to_render[1000];
local_only uint32_t zpolygons_to_render_size;
local_only zLightSource zlights_to_apply[50];
local_only uint32_t zlights_to_apply_size;
local_only zVertex camera = {0.0f, 0.0f, 0.0f};

extern char * texture_filenames[20];
extern DecodedImage * textures[20];
extern uint32_t texture_count;

void init_renderer(void);
void free_renderer(void);

void software_render(
    Vertex * next_gpu_workload,
    uint32_t * next_gpu_workload_size);

void draw_triangle(
    Vertex * vertices_recipient,
    uint32_t * vertex_count_recipient,
    Vertex input[3]);

void rotate_triangle(
    Vertex to_rotate[3],
    const float angle);

void translate_triangle(
    Vertex to_translate[3],
    const float x,
    const float y,
    const float z);

#endif

