#include "software_renderer.h"
#include "stdio.h"
#include "window_size.h"

void renderer_init() {
    box = get_box();
}

void software_render(
    ColoredVertex * next_gpu_workload,
    uint32_t * next_gpu_workload_size)
{
    // box->x -= 0.00001f;
    // box->y += 0.00001f;
    box->z += 0.001f;
    
    x_rotate_zpolygon(box, 0.001f);
    
    simd_float2 position = { box->x, box->y };
    
    // for (int i = 0; i < 6; i += 3) {
    for (int i = 0; i < box->vertices_size; i += 3) {
        ColoredVertex triangle_to_draw[3];
        
        simd_float4 triangle_color = {
            (i / 3) * (1.0f / box->vertices_size),
            (i / 3) * (1.0f / box->vertices_size),
            1.0f - i / 3,
            1.0f};
        
        ztriangle_to_2d(
            /* recipient: */ triangle_to_draw,
            /* input: */ box->triangle_vertices + i,
            /* x_offset: */ box->x,
            /* y_offset: */ box->y,
            /* z_offset: */ box->z,
            /* color: */ triangle_color);
        
        printf("after transforming:\n");
        printf(
            "<%f, %f>,",
            triangle_to_draw[0].XY[0],
            triangle_to_draw[0].XY[1]);
        printf(
            "<%f, %f>,",
            triangle_to_draw[1].XY[0],
            triangle_to_draw[1].XY[1]);
        printf(
            "<%f, %f>\n",
            triangle_to_draw[2].XY[0],
            triangle_to_draw[2].XY[1]);
        
        draw_triangle(
            /* vertices_recipient: */ next_gpu_workload,
            /* vertex_count_recipient: */ next_gpu_workload_size,
            /* input: */ triangle_to_draw,
            /* position: */ position);
    }
}

void draw_triangle(
    ColoredVertex * vertices_recipient,
    uint32_t * vertex_count_recipient,
    ColoredVertex input[3],
    simd_float2 position)
{
    uint32_t vertex_i = *vertex_count_recipient;
    
    vertices_recipient[vertex_i] = input[0];
    vertices_recipient[vertex_i].XY[0] += position[0];
    vertices_recipient[vertex_i].XY[1] += position[1];
    vertices_recipient[vertex_i].XY[0] *= WINDOW_WIDTH;
    vertices_recipient[vertex_i].XY[1] *= WINDOW_HEIGHT;
    vertex_i++;
    
    vertices_recipient[vertex_i] = input[1];
    vertices_recipient[vertex_i].XY[0] += position[0];
    vertices_recipient[vertex_i].XY[1] += position[1];
    vertices_recipient[vertex_i].XY[0] *= WINDOW_WIDTH;
    vertices_recipient[vertex_i].XY[1] *= WINDOW_HEIGHT;
    vertex_i++;
    
    vertices_recipient[vertex_i] = input[2];
    vertices_recipient[vertex_i].XY[0] += position[0];
    vertices_recipient[vertex_i].XY[1] += position[1];
    vertices_recipient[vertex_i].XY[0] *= WINDOW_WIDTH;
    vertices_recipient[vertex_i].XY[1] *= WINDOW_HEIGHT;
    vertex_i++;
    
    *vertex_count_recipient += 3;
}

void rotate_triangle(
    ColoredVertex to_rotate[3],
    const float angle)
{
    for (uint32_t i = 0; i < 3; i++) {
        to_rotate[i].XY[0] = 
            (cos(angle) * to_rotate[i].XY[0])
                + (sin(angle) * to_rotate[i].XY[1]);
        to_rotate[i].XY[1] =
            (-sin(angle) * to_rotate[i].XY[0])
                + (cos(angle) * to_rotate[i].XY[1]);
    }
}

