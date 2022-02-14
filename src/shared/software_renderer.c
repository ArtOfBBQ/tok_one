#include "software_renderer.h"
#include "assert.h"
#include "stdio.h"

void renderer_init() {
    printf("initializing software_renderer...\n");
    // box = get_box();
    box = load_from_obj_file("teddybear.obj");
    
    printf("triangles in object: %u\n", box->triangles_size);
    for (uint32_t i = 0; i < box->triangles_size; i++) {
        
        printf(
            "{%f,%f,%f},{%f,%f,%f},{%f,%f,%f}\n",
            box->triangles[i].vertices[0].x,
            box->triangles[i].vertices[0].y,
            box->triangles[i].vertices[0].z,
            box->triangles[i].vertices[1].x,
            box->triangles[i].vertices[1].y,
            box->triangles[i].vertices[1].z,
            box->triangles[i].vertices[2].x,
            box->triangles[i].vertices[2].y,
            box->triangles[i].vertices[2].z);

        if (
            box->triangles[i].vertices[0].x ==
                box->triangles[i].vertices[1].x
            && box->triangles[i].vertices[0].y ==
                box->triangles[i].vertices[1].y
            && box->triangles[i].vertices[0].z ==
                box->triangles[i].vertices[1].z)
        {
            printf(
                "error: vertices 0 & 1 at triangle %u match perfectly...\n",
                i);
            assert(0);
        }
        if (
            box->triangles[i].vertices[0].x ==
                box->triangles[i].vertices[2].x
            && box->triangles[i].vertices[0].y ==
                box->triangles[i].vertices[2].y
            && box->triangles[i].vertices[0].z ==
                box->triangles[i].vertices[2].z)
        {
            printf(
                "error: vertices 0 & 2 at triangle %u match perfectly...\n",
                i);
            assert(0);
        }
    }
}

void software_render(
    ColoredVertex * next_gpu_workload,
    uint32_t * next_gpu_workload_size)
{
    if (next_gpu_workload == NULL) {
        printf(
            "error - software_renderer got empty gpu_workload\n");
        return;
    }
    
    // box->x += 0.001f;
    // box->y -= 0.001f;
    
    box->x_angle += 0.08f;
    if (box->x_angle > 6.28) { 
        box->x_angle = 0.0f;
    }
    
    box->y_angle += 0.003f;
    if (box->y_angle > 6.28) { 
        box->y_angle = 0.0f;
    }
    
    box->z_angle += 0.005f;
    if (box->z_angle > 6.28) {
        box->z_angle = 0.0f;
    }
    
    if (box->triangles_size == 0) { return; }
    
    // x-rotate all triangles
    zTriangle x_rotated_triangles[box->triangles_size];
    for (uint32_t i = 0; i < box->triangles_size; i++) {
        x_rotated_triangles[i] =
            x_rotate_triangle(
                box->triangles + i,
                box->x_angle); 
    }
    
    // z-rotate all triangles
    zTriangle z_rotated_triangles[box->triangles_size];
    for (uint32_t i = 0; i < box->triangles_size; i++) {
        z_rotated_triangles[i] =
            z_rotate_triangle(
                &x_rotated_triangles[i],
                box->z_angle); 
    }
    
    // y-rotate all triangles
    zTriangle triangles_to_draw[box->triangles_size];
    for (uint32_t i = 0; i < box->triangles_size; i++) {
        triangles_to_draw[i] =
            y_rotate_triangle(
                &z_rotated_triangles[i],
                box->y_angle); 
    }
    
    // TODO: is this still necessary after we get normals? 
    // sort all triangles so the most distant ones can be
    // drawn first 
    z_sort(
        &triangles_to_draw[0],
        box->triangles_size);
    
    simd_float2 position = { box->x, box->y };
   
    printf("box->triangles_size: %u\n", box->triangles_size);   
    for (
        uint32_t i = box->triangles_size - 1;
        i > 0;
        i--)
    {
        printf("triangle i: %u\n", i);
        assert(i < box->triangles_size);
        for (uint32_t j = 0; j < 3; j++) {
            assert(box->triangles[i].vertices[j].x >= -40.0f);
            assert(box->triangles[i].vertices[j].x < 40.0f);
            assert(box->triangles[i].vertices[j].y >= -40.0f);
            assert(box->triangles[i].vertices[j].y < 40.0f);
            assert(box->triangles[i].vertices[j].z >= -40.0f);
            assert(box->triangles[i].vertices[j].z < 40.0f);
        }
        
        // calculate the normal, needed to determine if
        // triangle is visible
        // uses vector 'cross product'
        zVertex normal;
        zVertex line1;
        zVertex line2;
        
        // note to self: copied exactly from OLC vid
        line1.x = triangles_to_draw[i].vertices[1].x
            - triangles_to_draw[i].vertices[0].x;
        assert(line1.x < 40.0f);
        assert(line1.x >= -40.0f);
        line1.y = triangles_to_draw[i].vertices[1].y
            - triangles_to_draw[i].vertices[0].y;
        assert(line1.y < 40.0f);
        assert(line1.y >= -40.0f);
        line1.z = triangles_to_draw[i].vertices[1].z
            - triangles_to_draw[i].vertices[0].z;
        assert(line1.z < 40.0f);
        assert(line1.z >= -40.0f);
        
        line2.x = triangles_to_draw[i].vertices[2].x
            - triangles_to_draw[i].vertices[0].x;
        assert(line2.x < 40.0f);
        assert(line2.x >= -40.0f);
        line2.y = triangles_to_draw[i].vertices[2].y
            - triangles_to_draw[i].vertices[0].y;
        assert(line2.y < 40.0f);
        assert(line2.y >= -40.0f);
        line2.z = triangles_to_draw[i].vertices[2].z
            - triangles_to_draw[i].vertices[0].z;
        assert(line2.z < 40.0f);
        assert(line2.z >= -40.0f);
        
        // note to self: this is copied exactly from OLC vid
        normal.x = line1.y * line2.z - line1.z * line2.y;
        assert(normal.x > -40.0f);
        assert(normal.x < 40.0f);
        normal.y = line1.z * line2.x - line1.x * line2.z;
        assert(normal.y > -40.0f);
        assert(normal.y < 40.0f);
        normal.z = line1.x * line2.y - line1.y * line2.x;
        assert(normal.z > -40.0f);
        assert(normal.z < 40.0f);
        
        // note: this is copied exactly from the OLC vid
        printf("normal.x before sumsquares: %f\n", normal.x);
        printf("normal.x before sumsquares: %f\n", normal.y);
        printf("normal.x before sumsquares: %f\n", normal.z);
        float sum_squares =
            normal.x * normal.x +
            normal.y * normal.y +
            normal.z * normal.z;
        printf("sum_squares: %f\n", sum_squares);
        assert(sum_squares > 0.0f);
        float l = sqrtf(sum_squares);
        assert(l > 0.0f);
        printf("l: %f\n", l);
        normal.x /= l; normal.y /= l; normal.z /= l;
        assert(normal.x > -40.0f);
        assert(normal.x < 40.0f);
        assert(normal.y > -40.0f);
        assert(normal.y < 40.0f);
        assert(normal.z > -40.0f);
        assert(normal.z < 40.0f);
        
        // compare normal's similarity a point between
        // camera & triangle location 
        printf(
            "normal.x: %f, normal.y: %f, normal.z: %f\n",
            normal.x,
            normal.y,
            normal.z);
        
        float dot_x = (normal.x *
            (triangles_to_draw[i].vertices[0].x - camera.x));
        float dot_y = (normal.y * 
            (triangles_to_draw[i].vertices[0].y - camera.y));
        float dot_z = (normal.z *
            (triangles_to_draw[i].vertices[0].z - camera.z));
        float perspective_dot_product =
            dot_x + dot_y + dot_z;
        
        printf("dot_x: %f\n", dot_x);
        printf("dot_y: %f\n", dot_y);
        printf("dot_z: %f\n", dot_z);
        printf(
            "perspective_dot_product: %f\n",
            perspective_dot_product);
        
        // if (perspective_dot_product < 0.1f) {
        if (normal.z < 0.0f) {
            ColoredVertex triangle_to_draw[3];
            
            float avg_z = get_avg_z(triangles_to_draw + i);
            assert(far > 10.0f);
            float dist_modifier = ((far - box->z) / far);
            // assert(dist_modifier > 0.0f);
            // assert(dist_modifier < 1.0f);
            float brightness =
                0.95f - (
                    (avg_z + (dist_modifier * 10.0f)) / 11.0f);
            
            simd_float4 triangle_color = {
                brightness / 1.2f,
                brightness / 1.5f,
                brightness + 0.6f - (i * 0.04f),
                1.0f};
            
            ztriangle_to_2d(
                /* recipient: */ triangle_to_draw,
                /* input: */ triangles_to_draw + i,
                /* x_offset: */ box->x,
                /* y_offset: */ box->y,
                /* z_offset: */ box->z,
                /* color: */ triangle_color);
            
            draw_triangle(
                /* vertices_recipient: */
                    next_gpu_workload,
                /* vertex_count_recipient: */
                    next_gpu_workload_size,
                /* input: */
                    triangle_to_draw,
                /* position: */
                    position);
        }
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
            (cosf(angle) * to_rotate[i].XY[0])
                + (sinf(angle) * to_rotate[i].XY[1]);
        to_rotate[i].XY[1] =
            (-sinf(angle) * to_rotate[i].XY[0])
                + (cosf(angle) * to_rotate[i].XY[1]);
    }
}

