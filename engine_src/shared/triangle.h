#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "common.h"

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

void normalize_zvertex(
    zVertex * to_normalize);

#ifdef __cplusplus
}
#endif

#endif // TRIANGLE_H

