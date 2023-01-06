#include "draw_triangle.h"

void draw_vertices(
    GPU_Vertex * vertices_recipient,
    uint32_t * vertex_count_recipient,
    GPU_Vertex * input,
    const uint32_t input_size)
{
    uint32_t vertex_i = *vertex_count_recipient;
    
    for (uint32_t i = 0; i < input_size; i++) {
        assert(vertex_i + 1 < MAX_VERTICES_PER_BUFFER);
        vertices_recipient[vertex_i] = input[i];
        vertex_i++;
        *vertex_count_recipient += 1;
    }
}

static float get_triangle_area(
    const float x1,
    const float y1,
    const float x2,
    const float y2,
    const float x3,
    const float y3)
{
    float return_value = ( (x2-x1)*(y3-y1) - (x3-x1)*(y2-y1) );
    
    return return_value + ((return_value < 0) * (return_value * -2)); 
}

static bool32_t point_collides_triangle_area(
    const float normalized_x,
    const float normalized_y,
    const TriangleArea * area)
{
    float original_triangle_area = get_triangle_area(
        /* x1: */ area->viewport_x[0],
        /* y1: */ area->viewport_y[0],
        /* x2: */ area->viewport_x[1],
        /* y2: */ area->viewport_y[1],
        /* x3: */ area->viewport_x[2],
        /* y3: */ area->viewport_y[2]);
    
    float inner_triangle1_area = get_triangle_area(
        /* x1: */ normalized_x,
        /* y1: */ normalized_y,
        /* x2: */ area->viewport_x[1],
        /* y2: */ area->viewport_y[1],
        /* x3: */ area->viewport_x[2],
        /* y3: */ area->viewport_y[2]);
    
    float inner_triangle2_area = get_triangle_area(
        /* x1: */ area->viewport_x[0],
        /* y1: */ area->viewport_y[0],
        /* x2: */ normalized_x,
        /* y2: */ normalized_y,
        /* x3: */ area->viewport_x[2],
        /* y3: */ area->viewport_y[2]);

    float inner_triangle3_area = get_triangle_area(
        /* x1: */ area->viewport_x[0],
        /* y1: */ area->viewport_y[0],
        /* x2: */ area->viewport_x[1],
        /* y2: */ area->viewport_y[1],
        /* x3: */ normalized_x,
        /* y3: */ normalized_y);
    
    bool32_t result = fabs((inner_triangle1_area +
        inner_triangle2_area +
        inner_triangle3_area)
            - original_triangle_area) < 0.0001f;
    
    return result;
}

