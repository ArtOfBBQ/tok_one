#ifndef BOX_H
#define BOX_H

#include "vertex_types.h"
#include "bool_types.h"
#include "window_size.h"

// projection matrix constants
float near;
float far;
float z_normalisation;
float field_of_view;
float field_of_view_angle;
float field_of_view_rad;
float field_of_view_modifier;
float aspect_ratio;

void z_constants_init();

typedef struct zPolygonVertex {
    float x;
    float y;
    float z;
} zPolygonVertex;

typedef struct zPolygon {
    zPolygonVertex * triangle_vertices;
    uint32_t vertices_size;
    simd_float4 * triangle_colors;
    uint32_t triangles_size;
    float x;
    float y;
    float z;
} zPolygon;

zPolygon * get_box();

void ztriangle_to_2d(
    ColoredVertex recipient[3],
    zPolygonVertex * input,
    float x_offset,
    float y_offset,
    float z_offset,
    simd_float4 color);

void x_rotate_zpolygon(
    zPolygon * to_rotate,
    const float angle);

void y_rotate_zpolygon(
    zPolygon * to_rotate,
    const float angle);

void free_zpolygon(
    zPolygon * to_free);

float get_avg_z(
    zPolygon * of_zpolygon,
    uint32_t at_i);

void z_sort(
    zPolygon * to_sort);

#endif

