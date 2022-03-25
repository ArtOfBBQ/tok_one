#ifndef DRAW_TRIANGLE_H
#define DRAW_TRIANGLE_H

#include <stdio.h>
#include <assert.h>

#include "common.h"
#include "vertex_types.h"

void draw_triangle(
    Vertex * vertices_recipient,
    uint32_t * vertex_count_recipient,
    Vertex input[3]);

#endif

