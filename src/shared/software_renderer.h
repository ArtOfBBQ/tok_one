#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include "inttypes.h"

#include "common.h"
#include "window_size.h"
#include "platform_layer.h"
#include "vertex_types.h"
#include "draw_triangle.h"
#include "zpolygon.h"
#include "decode_png.h"
#include "bitmap_renderer.h"

local_only uint32_t renderer_initialized = false;
extern zPolygon * zpolygons_to_render[1000];
extern uint32_t zpolygons_to_render_size;
local_only zLightSource zlights_to_apply[50];
local_only uint32_t zlights_to_apply_size;

typedef struct TextureArray {
    char * filename;
    DecodedImage * image;
    uint32_t sprite_columns;
    uint32_t sprite_rows;
} TextureArray;

extern TextureArray texture_arrays[TEXTUREARRAYS_SIZE + 1];

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

zPolygon * load_from_obj_file(char * filename);

#endif

