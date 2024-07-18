#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "common.h"
#include "logger.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 0
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
#endif

void normalize_zvertex_f3(
    float to_normalize_xyz[3]);
void normalize_vertex(
    float * to_normalize);

void x_rotate_f3(
    float * vertices,
    float x_angle);

#ifdef __cplusplus
}
#endif

#endif // TRIANGLE_H

