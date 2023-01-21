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
