#include "software_renderer.h"
#include "stdio.h"

void software_render(
    ColoredVertex * next_gpu_workload,
    uint32_t * next_gpu_workload_size)
{
    // ColoredVertex triangle_top[3];
    // triangle_top[0].XY =
    //     (simd_float2){ 50.0f, 100.0f };
    // triangle_top[0].RGBA =
    //     (simd_float4){ 1.0f, 0.0f, 1.0f, 1.0f };
    // triangle_top[1].XY =
    //     (simd_float2){ 0.0f, 0.0f };
    // triangle_top[1].RGBA =
    //     (simd_float4){ 1.0f, 0.0f, 1.0f, 1.0f };
    // triangle_top[2].XY =
    //     (simd_float2){ 50.0f, 0.0f };
    // triangle_top[2].RGBA =
    //     (simd_float4){ 1.0f, 1.0f, 0.5f, 1.0f };
    // 
    // simd_float2 triangle_top_pos = { 50.0f, 250.0f };
    // simd_float2 triangle_bottom_pos = { 400.0f, 950.0f };
    // draw_triangle(
    //     /* vertices_recipient: */ next_gpu_workload,
    //     /* vertex_count_recipient: */ next_gpu_workload_size,
    //     /* input: */ triangle_top,
    //     /* position: */ triangle_top_pos);
    // draw_triangle(
    //     /* vertices_recipient: */ next_gpu_workload,
    //     /* vertex_count_recipient: */ next_gpu_workload_size,
    //     /* input: */ triangle_top,
    //     /* position: */ triangle_bottom_pos);
    // 
    // return;
    zPolygon * box = malloc(sizeof(zPolygon));
    box->triangles_size = 6 * 2; // 6 faces, 2 per face
    box->vertices_size = box->triangles_size * 3;
    box->triangle_vertices = malloc(
        sizeof(zPolygonVertex) * box->vertices_size);
    
    box->x = 50.0f;
    box->y = 500.0f;
    box->z = 1.5f;

    // north face
    // (exactly the same values as south,
    // except z is always distant instead of close
    box->triangle_vertices[0] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.15f };
    box->triangle_vertices[1] =
        (zPolygonVertex){ 0.0f, 250.0f, 0.15f };
    box->triangle_vertices[2] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.15f };
    
    box->triangle_vertices[3] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.15f };
    box->triangle_vertices[4] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.15f };
    box->triangle_vertices[5] =
        (zPolygonVertex){ 250.0f, 0.0f, 0.15f };
    
    // bottom face
    box->triangle_vertices[6] =
        (zPolygonVertex){ 250.0f, 0.0f, 0.15f };
    box->triangle_vertices[7] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.15f };
    box->triangle_vertices[8] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    
    box->triangle_vertices[9] =
        (zPolygonVertex){ 250.0f, 0.0f, 0.15f };
    box->triangle_vertices[10] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangle_vertices[11] =
        (zPolygonVertex){ 250.0f, 0.0f, 0.0f };
    
    // west face (like east, but x is always 0 instead of 1)
    box->triangle_vertices[12] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangle_vertices[13] =
        (zPolygonVertex){ 0.0f, 250.0f, 0.0f };
    box->triangle_vertices[14] =
        (zPolygonVertex){ 0.0f, 250.0f, 0.15f };
    
    box->triangle_vertices[15] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangle_vertices[16] =
        (zPolygonVertex){ 0.0f, 250.0f, 0.15f };
    box->triangle_vertices[17] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.15f };
    
    // east face
    box->triangle_vertices[18] =
        (zPolygonVertex){ 250.0f, 0.0f, 0.0f };
    box->triangle_vertices[19] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.0f };
    box->triangle_vertices[20] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.15f };
    
    box->triangle_vertices[21] =
        (zPolygonVertex){ 250.0f, 0.0f, 0.0f };
    box->triangle_vertices[22] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.15f };
    box->triangle_vertices[23] =
        (zPolygonVertex){ 250.0f, 0.0f, 0.15f };
    
    // top face
    box->triangle_vertices[24] =
        (zPolygonVertex){ 0.0f, 250.0f, 0.0f };
    box->triangle_vertices[25] =
        (zPolygonVertex){ 0.0f, 250.0f, 0.15f };
    box->triangle_vertices[26] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.15f };
    
    box->triangle_vertices[27] =
        (zPolygonVertex){ 0.0f, 250.0f, 0.0f };
    box->triangle_vertices[28] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.15f };
    box->triangle_vertices[29] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.0f };
    
    // box's south face (2 triangles)
    //                      x     y     z
    box->triangle_vertices[30] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangle_vertices[31] =
        (zPolygonVertex){ 0.0f, 250.0f, 0.0f };
    box->triangle_vertices[32] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.0f };
    
    box->triangle_vertices[33] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangle_vertices[34] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.0f };
    box->triangle_vertices[35] =
        (zPolygonVertex){ 250.0f, 0.0f, 0.0f };
    
    simd_float2 position = { box->x, box->y };
    simd_float4 lightblue = {
        0.5f,
        0.5f,
        1.0f,
        1.0f};
    simd_float4 midblue = {
        0.4f,
        0.4f,
        0.8f,
        1.0f};
    simd_float4 darkblue = {
        0.1f,
        0.1f,
        0.5f,
        1.0f};
    
    for (int i = 0; i < box->vertices_size; i += 3) {
        ColoredVertex triangle_to_draw[3];
        
        simd_float4 color = darkblue; 
        if (box->triangle_vertices[i].z < 0.14f
            && box->triangle_vertices[i+1].z < 0.14f
            && box->triangle_vertices[i+2].z < 0.14f)
        {
            color = lightblue;
        } else if (
            box->triangle_vertices[i].z < 0.14f
            || box->triangle_vertices[i+1].z < 0.14f
            || box->triangle_vertices[i+2].z < 0.14f)
        {
            color = midblue;
        }
        
        printf(
            "box at {%f, %f, %f}'s next triangle:\n",
            box->x,
            box->y,
            box->z);
        printf(
            "{%f, %f, %f},",
            box->triangle_vertices[i].x,
            box->triangle_vertices[i].y,
            box->triangle_vertices[i].z);
        printf(
            "{%f, %f, %f},",
            box->triangle_vertices[i+1].x,
            box->triangle_vertices[i+1].y,
            box->triangle_vertices[i+1].z);
        printf(
            "{%f, %f, %f}\n",
            box->triangle_vertices[i+2].x,
            box->triangle_vertices[i+2].y,
            box->triangle_vertices[i+2].z);
        
        ztriangle_to_2d(
            /* recipient: */ triangle_to_draw,
            /* input: */ box->triangle_vertices + i,
            /* x_offset: */ box->x,
            /* y_offset: */ box->y,
            /* z_offset: */ box->z,
            /* color: */ color);
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
    vertex_i++;
    
    vertices_recipient[vertex_i] = input[1];
    vertices_recipient[vertex_i].XY[0] += position[0];
    vertices_recipient[vertex_i].XY[1] += position[1];
    vertex_i++;
    
    vertices_recipient[vertex_i] = input[2];
    vertices_recipient[vertex_i].XY[0] += position[0];
    vertices_recipient[vertex_i].XY[1] += position[1];
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

