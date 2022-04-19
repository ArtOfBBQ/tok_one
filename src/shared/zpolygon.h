#ifndef ZPOLYGON_H
#define ZPOLYGON_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <inttypes.h>
#include <assert.h>

#include "common.h"
#include "vertex_types.h"
#include "window_size.h"

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

typedef struct zCamera {
    float x;
    float y;
    float z;
    float x_angle;
    float y_angle;
    float z_angle;
} zCamera;
extern zCamera camera;

typedef struct zLightSource {
    float x;
    float y;
    float z;
    float RGBA[4];
    float reach;   // max distance before light intensity 0
    float ambient; // how much ambient light does this radiate?
    float diffuse; // how much diffuse light does this radiate?
} zLightSource;

// A buffer of zLightSources to light up your scene(s)
// index 0 to zlights_to_apply_size will be rendered,
// the rest of the array will be ignored
#define ZLIGHTS_TO_APPLY_ARRAYSIZE 50
extern zLightSource zlights_to_apply[ZLIGHTS_TO_APPLY_ARRAYSIZE];
extern uint32_t zlights_to_apply_size;

typedef struct zVertex {
    float x;
    float y;
    float z;
    float uv[2];         // texture coords, ignored if untextured
} zVertex;

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

void ztriangle_apply_lighting(
    Vertex recipient[3],
    zTriangle * input,
    zLightSource * zlight_source);

void ztriangle_to_3d(
    Vertex recipient[3],
    zTriangle * input);

void ztriangle_to_2d(
    Vertex recipient[3],
    zTriangle * input);

zVertex x_rotate_zvertex(
    const zVertex * input,
    const float angle);
zTriangle x_rotate_ztriangle(
    const zTriangle * input,
    const float angle);
zVertex y_rotate_zvertex(
    const zVertex * input,
    const float angle);
zTriangle y_rotate_ztriangle(
    const zTriangle * input,
    const float angle);
zVertex z_rotate_zvertex(
    const zVertex * input,
    const float angle);
zTriangle z_rotate_ztriangle(
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

void zcamera_move_forward(
    zCamera * to_move,
    const float distance);

#endif
