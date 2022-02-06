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

typedef struct zTriangle {
    zPolygonVertex vertices[3];
} zTriangle;

typedef struct zPolygon {
    zTriangle * triangles;
    uint32_t triangles_size;
    float x;
    float y;
    float z;
    float x_angle;
    float y_angle;
    float z_angle;
} zPolygon;

zPolygon * get_box();

void ztriangle_to_2d(
    ColoredVertex recipient[3],
    zTriangle * input,
    float x_offset,
    float y_offset,
    float z_offset,
    simd_float4 color);

zTriangle x_rotate_triangle(
    const zTriangle * input, const float angle);
zTriangle y_rotate_triangle(
    const zTriangle * input, const float angle);
zTriangle z_rotate_triangle(
    const zTriangle * input, const float angle);

void free_zpolygon(
    zPolygon * to_free);

float get_avg_z(const zTriangle * of_triangle);

void z_sort(
    zTriangle * triangles,
    const uint32_t triangles_size);

#endif

