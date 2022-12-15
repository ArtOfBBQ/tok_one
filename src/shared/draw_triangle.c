#include "draw_triangle.h"

TriangleArea * touchable_triangles;
uint32_t touchable_triangles_size = 0;

void __attribute__((no_instrument_function))
draw_triangle(
    Vertex * vertices_recipient,
    uint32_t * vertex_count_recipient,
    Vertex input[3],
    int32_t touchable_id)
{
    log_assert(vertices_recipient != NULL);
    
    uint32_t vertex_i = *vertex_count_recipient;
    
    for (uint32_t i = 0; i < 3; i++) {
        vertices_recipient[vertex_i] = input[i];
        vertex_i++;
    }
    *vertex_count_recipient += 3;
    
    if (touchable_id >= 0) {
        log_assert(touchable_triangles_size < TOUCHABLE_TRIANGLES_ARRAYSIZE);
        
        for (
             uint32_t v = 0;
             v < 3;
             v++)
        {
            touchable_triangles[touchable_triangles_size]
                .viewport_x[v] = input[v].x / input[v].w;
            touchable_triangles[touchable_triangles_size]
                .viewport_y[v] = input[v].y / input[v].w;
            touchable_triangles[touchable_triangles_size]
                .touchable_id = touchable_id;
            touchable_triangles[touchable_triangles_size]
                .viewport_z[v] = input[v].z;
        }
        
        touchable_triangles_size++;
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

int32_t find_touchable_at(
    const float x,
    const float y)
{
    int32_t return_value = -1;
    float current_z = FLOAT32_MAX;
    
    for (
        int32_t i = (int32_t)(touchable_triangles_size - 1);
        i >= 0;
        i--)
    {
        float avg_z = (
            touchable_triangles[i].viewport_z[0] +
            touchable_triangles[i].viewport_z[1] +
            touchable_triangles[i].viewport_z[2]) / 3;
        if (
            avg_z < current_z &&
            point_collides_triangle_area(
                /* normalized_x : */ x,
                /* normalized_y : */ y,
                /* TriangleArea * area : */ &touchable_triangles[i]))
        {
            current_z = avg_z;
            return_value = touchable_triangles[i].touchable_id;
        }
    }
    
    return return_value;
}
