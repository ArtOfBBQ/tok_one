#ifndef ZPOLYGON_H
#define ZPOLYGON_H

// TODO: remove std lib
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>

#include "logger.h"
#include "common.h"
#include "vertex_types.h"
#include "lightsource.h"
#include "window_size.h"

#ifdef __cplusplus
extern "C" {
#endif

// projection constants
typedef struct ProjectionConstants {
    float near;
    float far;
    float field_of_view_rad;
    float field_of_view_modifier;
    float aspect_ratio;
} ProjectionConstants;

extern ProjectionConstants projection_constants;

void init_projection_constants(void);

void zcamera_move_forward(
    zCamera * to_move,
    const float distance);

typedef struct zTriangle {
    zVertex vertices[3];
    float color[4];         // RGBA, ignored if textured
    int32_t texturearray_i; /*
                            the index in the global var
                            'texture_arrays' of the texturearray
                             -1 for "untextured, use color
                            instead"
                            */
    int32_t texture_i;     // index in texturearray
    uint32_t draw_normals; // TODO: remove debugging flag(s)
    uint32_t visible;
} zTriangle;

typedef struct zPolygon {
    uint32_t object_id;
    zTriangle * triangles;
    uint32_t triangles_size;
    float x;
    float y;
    float z;
    float x_angle;
    float y_angle;
    float z_angle;
} zPolygon;

// A buffer of zPolygon objects that should be rendered
// in your application
// index 0 to zpolygons_to_render_size will be rendered,
// the rest of the array will be ignored
#define ZPOLYGONS_TO_RENDER_ARRAYSIZE 2
extern zPolygon zpolygons_to_render[ZPOLYGONS_TO_RENDER_ARRAYSIZE];
extern uint32_t zpolygons_to_render_size;

void __attribute__((no_instrument_function))
ztriangle_apply_lighting(
    Vertex recipient[3],
    zTriangle * input,
    zLightSource * zlight_source);

void ztriangle_to_3d(
    Vertex recipient[3],
    zTriangle * input);

void ztriangle_to_2d(
    Vertex recipient[3],
    zTriangle * input);

zTriangle __attribute__((no_instrument_function))
x_rotate_ztriangle(
    const zTriangle * input,
    const float angle);
zTriangle __attribute__((no_instrument_function))
y_rotate_ztriangle(
    const zTriangle * input,
    const float angle);
zTriangle __attribute__((no_instrument_function))
z_rotate_ztriangle(
    const zTriangle * input,
    const float angle);

zTriangle translate_ztriangle(
    const zTriangle * input,
    const float by_x,
    const float by_y,
    const float by_z);

zVertex get_ztriangle_normal(
    const zTriangle * input,
    const uint32_t at_vertex_i);

zPolygon parse_obj(
    char * rawdata,
    uint64_t rawdata_size);

void scale_zpolygon(
    zPolygon * to_scale,
    const float new_height);

float get_avg_z(
    const zTriangle * of_triangle);

int sorter_cmpr_lowest_z(
    const void * a,
    const void * b);

void z_sort(
    zTriangle * triangles,
    const uint32_t triangles_size);

void normalize_zvertex(
    zVertex * to_normalize);

float get_distance(
    const zVertex p1,
    const zVertex p2);

float distance_to_zvertex(
    const zVertex p1,
    const zVertex p2);

float distance_to_ztriangle(
    const zVertex p1,
    const zTriangle p2);

float get_visibility_rating(
    const zVertex observer,
    const zTriangle * observed,
    const uint32_t observed_vertex_i);

float dot_of_vertices(
    const zVertex vertex_1,
    const zVertex vertex_2);
    
#ifdef __cplusplus
}
#endif

#endif // ZPOLYGON_H
