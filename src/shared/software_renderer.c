#include "software_renderer.h"

void init_renderer() {
    // box = get_box();
    
    box = load_from_obj_file("teddybear.obj");
}

void free_renderer() {
    free_zpolygon(box);
}

void software_render(
    ColoredVertex * next_gpu_workload,
    uint32_t * next_gpu_workload_size)
{
    if (next_gpu_workload == NULL) {
        return;
    }
    
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
    
    if (box->triangles_size == 0) {
        return;
    }
    
    // x-rotate all triangles
    zTriangle x_rotated_triangles[box->triangles_size];
    for (uint32_t i = 0; i < box->triangles_size; i++) {
        x_rotated_triangles[i] =
            x_rotate_triangle(
                box->triangles + i,
                box->x_angle); 
    }
    
    zTriangle z_rotated_triangles[box->triangles_size];
    for (uint32_t i = 0; i < box->triangles_size; i++) {
        z_rotated_triangles[i] =
            z_rotate_triangle(
                &x_rotated_triangles[i],
                box->z_angle); 
    }
    
    zTriangle y_rotated_triangles[box->triangles_size];
    for (uint32_t i = 0; i < box->triangles_size; i++) {
        y_rotated_triangles[i] =
            y_rotate_triangle(
                &z_rotated_triangles[i],
                box->y_angle);
    }
   
    zTriangle triangles_to_draw[box->triangles_size];
    for (uint32_t i = 0; i < box->triangles_size; i++) {
        triangles_to_draw[i] = 
            translate_ztriangle(
                &y_rotated_triangles[i],
                /* x: */ box->x,
                /* y: */ box->y,
                /* z: */ box->z);
    }
    
    // sort all triangles so the most distant ones can be
    // drawn first 
    z_sort(
        &triangles_to_draw[0],
        box->triangles_size);
     
    for (
        int32_t i = box->triangles_size - 1;
        i >= 0;
        i -= 1)
    {
        // calculate the normal, needed to determine if
        // triangle is visible
        // uses vector 'cross product'
        zVertex normal;
        zVertex line1;
        zVertex line2;
        
        // note to self: copied exactly from OLC vid
        line1.x =
            triangles_to_draw[i].vertices[1].x
                - triangles_to_draw[i].vertices[0].x;
        line1.y =
            triangles_to_draw[i].vertices[1].y
                - triangles_to_draw[i].vertices[0].y;
        line1.z =
            triangles_to_draw[i].vertices[1].z
                - triangles_to_draw[i].vertices[0].z;
        
        line2.x =
            triangles_to_draw[i].vertices[2].x
                - triangles_to_draw[i].vertices[0].x;
        line2.y =
            triangles_to_draw[i].vertices[2].y
                - triangles_to_draw[i].vertices[0].y;
        line2.z =
            triangles_to_draw[i].vertices[2].z
                - triangles_to_draw[i].vertices[0].z;
        
        // note to self: this is copied exactly from OLC vid
        normal.x =
            (line1.y * line2.z) - (line1.z * line2.y);
        normal.y =
            (line1.z * line2.x) - (line1.x * line2.z);
        normal.z =
            (line1.x * line2.y) - (line1.y * line2.x);
        
        // note: this is copied exactly from the OLC vid
        float sum_squares =
            (normal.x * normal.x) +
            (normal.y * normal.y) +
            (normal.z * normal.z);
        float length = sqrtf(sum_squares);
        normal.x /= length;
        normal.y /= length;
        normal.z /= length;
        
        // compare normal's similarity a point between
        // camera & triangle location 
        float dot_x = (normal.x *
            (triangles_to_draw[i].vertices[0].x - camera.x));
        float dot_y = (normal.y * 
            (triangles_to_draw[i].vertices[0].y - camera.y));
        float dot_z = (normal.z *
            (triangles_to_draw[i].vertices[0].z - camera.z));
        float perspective_dot_product =
            dot_x + dot_y + dot_z;
        
        if (perspective_dot_product < 0.0f) {
            ColoredVertex triangle_to_draw[3];
            
            float avg_z =
                get_avg_z(triangles_to_draw + i);
            float dist_modifier = avg_z / far;
            // assert(dist_modifier > 0.0f);
            // assert(dist_modifier < 1.0f);
            float brightness =
                0.85f - dist_modifier;
            
            float triangle_color[4] = {
                fmin(0.35 + brightness, 1.0f),
                fmin(0.20 + brightness, 1.0f),
                fmin(0.15 + brightness, 1.0f),
                1.0f};
            
            ztriangle_to_2d(
                /* recipient: */ triangle_to_draw,
                /* input: */ triangles_to_draw + i,
                /* color: */ triangle_color);
            
            draw_triangle(
                /* vertices_recipient: */
                    next_gpu_workload,
                /* vertex_count_recipient: */
                    next_gpu_workload_size,
                /* input: */
                    triangle_to_draw);
        }
    }
}

void draw_triangle(
    ColoredVertex * vertices_recipient,
    uint32_t * vertex_count_recipient,
    ColoredVertex input[3])
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

void rotate_triangle(
    ColoredVertex to_rotate[3],
    const float angle)
{
    for (uint32_t i = 0; i < 3; i++) {
        to_rotate[i].x = 
            (cosf(angle) * to_rotate[i].x)
                + (sinf(angle) * to_rotate[i].y);
        to_rotate[i].y =
            (-sinf(angle) * to_rotate[i].x)
                + (cosf(angle) * to_rotate[i].y);
    }
}

