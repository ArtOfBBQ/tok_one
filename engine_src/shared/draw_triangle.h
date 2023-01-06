#ifndef DRAW_TRIANGLE_H
#define DRAW_TRIANGLE_H

/*
This header currently also has the code for mapping out
triangular areas of the screen marked as "clickable"
and so is poorly named

TODO: consider filename / organizational changes when
the architecture is decided
*/

#include <math.h>

#include "common.h"
#include "logger.h"
#include "cpu_gpu_shared_types.h"
#include "cpu_to_gpu_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TriangleArea {
    // by viewport i mean this coordinate system:
    //   * -1.0f is the left of the screen
    //   * 1.0f is the right of the screen
    //   * 1.0f is the top of the screen
    //   * -1.0f is the bottom of the screen
    float viewport_x[3];
    float viewport_y[3];
    float viewport_z[3];
    int32_t touchable_id;
} TriangleArea;

void draw_vertices(
    GPU_Vertex * vertices_recipient,
    uint32_t * vertex_count_recipient,
    GPU_Vertex * input,
    const uint32_t input_size);

#ifdef __cplusplus
}
#endif

#endif // DRAW_TRIANGLE_H
