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

//static float get_triangle_area(
//    const float x1,
//    const float y1,
//    const float x2,
//    const float y2,
//    const float x3,
//    const float y3)
//{
//    float return_value = ( (x2-x1)*(y3-y1) - (x3-x1)*(y2-y1) );
//    
//    return return_value + ((return_value < 0) * (return_value * -2)); 
//}
