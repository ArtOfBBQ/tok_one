#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "common.h"
#include "logger.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct zVertex {
    float x;
    float y;
    float z;
    float uv[2];         // texture coords, ignored if untextured
} zVertex;

typedef struct zTriangle {
    zVertex vertices[3];
    zVertex normal;
    int32_t parent_material_i;
} zTriangle;

void normalize_vertex(
    float * to_normalize);
void normalize_zvertex(
    zVertex * to_normalize);

zVertex crossproduct_of_zvertices(
    const zVertex * a,
    const zVertex * b);

float get_squared_distance(
    const zVertex a,
    const zVertex b);
float get_squared_triangle_length(
    const zTriangle * subject);
float get_triangle_area(
    const zTriangle * subject);

#ifdef __cplusplus
}
#endif

#endif // TRIANGLE_H

