#include "software_renderer.h"

// TODO: clean up global variables
char * texture_filenames[20];
DecodedImage * textures[20];
uint32_t texture_count = 0;

void init_renderer() {
    // initialize global textures for texture mapping 
    texture_count = 2;
    texture_filenames[0] = "fs_angrymob.png";
    texture_filenames[1] = "structuredart.png";
    
    FileBuffer * file_buffer;
    for (uint32_t i = 0; i < texture_count; i++) {
        file_buffer = platform_read_file(texture_filenames[i]);
        textures[i] = decode_PNG(
            (uint8_t *)file_buffer->contents,
            file_buffer->size);
        free(file_buffer->contents);
        free(file_buffer);
        printf(
            "read texture %s with width %u\n",
            texture_filenames[i],
            textures[i]->width);
    }
    
    // initialize zPolygon objects, the 3-D objects we're
    // planning to render
    zpolygons_to_render_size = 0;
  
    // load some teddybears from the object file 
    for (uint32_t i = 0; i < 2; i++) {
        zpolygons_to_render[i] =
            load_from_obj_file("teddybear.obj");
        zpolygons_to_render_size += 1;
        
        float base_x = i % 3 == 1 ? 0.0f : -40.0f; 
        float base_y = i % 2 == 1 ? 0.0f : -5.0f;
        zpolygons_to_render[i]->x = base_x + (i * 7.0f);
        zpolygons_to_render[i]->y = base_y + (i * 7.0f);
        zpolygons_to_render[i]->z = (90.0f + ((i/2) * 10.0f));
    }

    // load some hard-coded cubes
    for (uint32_t i = 2; i < 3; i++) {
        zpolygons_to_render[i] = get_box();
        zpolygons_to_render_size += 1;
        
        float base_x = i % 3 == 1 ? 0.0f : -40.0f; 
        float base_y = i % 2 == 1 ? 0.0f : -5.0f;
    }
}

void free_renderer() {
    for (uint32_t i = 0; i < zpolygons_to_render_size; i++) {
        free_zpolygon(zpolygons_to_render[i]);
    }
}

void software_render(
    Vertex * next_gpu_workload,
    uint32_t * next_workload_size)
{
    if (
        next_gpu_workload == NULL
        || next_workload_size == NULL)
    {
        printf("ERROR: platform layer didnt pass recipients\n");
        return;
    }
    
    if (zpolygons_to_render_size == 0) {
        return;
    }
    
    for (uint32_t i = 0; i < zpolygons_to_render_size; i++) {
        zpolygons_to_render[i]->x += 0.001;
        zpolygons_to_render[i]->y += 0.001;
        zpolygons_to_render[i]->z += 0.01;
        zpolygons_to_render[i]->x_angle += 0.04f;
        zpolygons_to_render[i]->y_angle += 0.04f;
    }
    
    uint32_t triangles_to_render = 0;
    for (uint32_t i = 0; i < zpolygons_to_render_size; i++) {
        for (
            uint32_t j = 0;
            j < zpolygons_to_render[i]->triangles_size;
            j++)
        {
            triangles_to_render++;
        }
    }
    
    if (triangles_to_render == 0) { return; }
    
    // rotate all triangles
    zTriangle triangles_to_draw[triangles_to_render];
    zTriangle x_rotated;
    zTriangle y_rotated;
    zTriangle z_rotated;
    uint32_t t = 0;
    for (uint32_t i = 0; i < zpolygons_to_render_size; i++) {
        for (
            uint32_t j = 0;
            j < zpolygons_to_render[i]->triangles_size;
            j++)
        {
            assert(t < triangles_to_render);
            x_rotated = x_rotate_triangle(
                zpolygons_to_render[i]->triangles + j,
                zpolygons_to_render[i]->x_angle);
            y_rotated = y_rotate_triangle(
                &x_rotated,
                zpolygons_to_render[i]->y_angle);
            z_rotated = z_rotate_triangle(
                &y_rotated,
                zpolygons_to_render[i]->z_angle);
            
            triangles_to_draw[t++] =
                translate_ztriangle(
                    &z_rotated,
                    /* x: */ zpolygons_to_render[i]->x,
                    /* y: */ zpolygons_to_render[i]->y,
                    /* z: */ zpolygons_to_render[i]->z);
        }
    }
    
    // sort all triangles so the most distant ones can be
    // drawn first 
    qsort(
        triangles_to_draw,
        triangles_to_render,
        sizeof(zTriangle),
        &sorter_cmpr_lowest_z);
    
    for (
        int32_t i = triangles_to_render - 1;
        i >= 0;
        i -= 1)
    {
        // calculate the normal, needed to determine if
        // triangle is visible
        // uses vector 'cross product'
        zVertex normal;
        zVertex line1;
        zVertex line2;
        
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
        
        normal.x =
            (line1.y * line2.z) - (line1.z * line2.y);
        normal.y =
            (line1.z * line2.x) - (line1.x * line2.z);
        normal.z =
            (line1.x * line2.y) - (line1.y * line2.x);
        
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
            // colored triangle
            Vertex triangle_to_draw[3];
           
            float avg_z =
                get_avg_z(triangles_to_draw + i);
            float dist_modifier =
                avg_z / projection_constants.far;
            float brightness =
                fmax(0.85f - dist_modifier, 0.0f);
            
            float triangle_color[4] = {
                fmin(0.25 + brightness, 1.0f),
                fmin(0.20 + brightness, 1.0f),
                fmin(0.15 + brightness, 1.0f),
                1.0f};
            
            triangles_to_draw[i].color[0] =
                triangle_color[0];
            triangles_to_draw[i].color[1] =
                triangle_color[1];
            triangles_to_draw[i].color[2] =
                triangle_color[2];
            triangles_to_draw[i].color[3] =
                triangle_color[3];

            triangle_to_draw[0].uv[0] = 0.0f;
            triangle_to_draw[0].uv[1] = 1.0f;
            triangle_to_draw[1].uv[0] = 0.0f;
            triangle_to_draw[1].uv[1] = 0.0f;
            triangle_to_draw[2].uv[0] = 1.0f;
            triangle_to_draw[2].uv[1] = 1.0f;
            
            ztriangle_to_2d(
                /* recipient: */
                    triangle_to_draw,
                /* input: */
                    triangles_to_draw + i);
            
            draw_triangle(
                /* vertices_recipient: */
                    next_gpu_workload,
                /* vertex_count_recipient: */
                    next_workload_size,
                /* input: */
                    triangle_to_draw);
        }
    }
}

void draw_triangle(
    Vertex * vertices_recipient,
    uint32_t * vertex_count_recipient,
    Vertex input[3])
{
    assert(vertices_recipient != NULL);
    
    uint32_t vertex_i = *vertex_count_recipient;
    
    vertices_recipient[vertex_i] = input[0];
    for (uint32_t i = 0; i < 4; i++) {
        vertices_recipient[vertex_i].RGBA[i] = 
            input->RGBA[i];
    }
    vertex_i++;
    
    vertices_recipient[vertex_i] = input[1];
    for (uint32_t i = 0; i < 4; i++) {
        vertices_recipient[vertex_i].RGBA[i] = 
            input->RGBA[i];
    }
    vertex_i++;
    
    vertices_recipient[vertex_i] = input[2];
    for (uint32_t i = 0; i < 4; i++) {
        vertices_recipient[vertex_i].RGBA[i] = 
            input->RGBA[i];
    }
    vertex_i++;
    
    *vertex_count_recipient += 3;
}

void rotate_triangle(
    Vertex to_rotate[3],
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

