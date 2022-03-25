#include "draw_triangle.h"

void draw_triangle(
    Vertex * vertices_recipient,
    uint32_t * vertex_count_recipient,
    Vertex input[3])
{
    assert(vertices_recipient != NULL);
    
    uint32_t vertex_i = *vertex_count_recipient;
    
    vertices_recipient[vertex_i] = input[0];
    vertex_i++;
    
    vertices_recipient[vertex_i] = input[1];
    vertex_i++;
    
    vertices_recipient[vertex_i] = input[2];
    vertex_i++;
    
    *vertex_count_recipient += 3;
}

