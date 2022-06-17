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
#include "vertex_types.h"

typedef struct TriangleArea {
    // by normalized i mean this coordinate system:
    //   * -1.0f is the left of the screen
    //   * 1.0f is the right of the screen
    //   * 1.0f is the top of the screen
    //   * -1.0f is the bottom of the screen
    float normalized_x[3];
    float normalized_y[3];
    int32_t touchable_id;
} TriangleArea;

#define TOUCHABLE_TRIANGLES_ARRAYSIZE 1500
extern TriangleArea touchable_triangles[
    TOUCHABLE_TRIANGLES_ARRAYSIZE];
extern uint32_t touchable_triangles_size;

void __attribute__((no_instrument_function))
draw_triangle(
    Vertex * vertices_recipient,
    uint32_t * vertex_count_recipient,
    Vertex input[3]);

void register_touchable_triangle(
    const int32_t touchable_id,
    Vertex triangle_area[3]);

int32_t find_touchable_at(
    const float normalized_x,
    const float normalized_y);

#endif

