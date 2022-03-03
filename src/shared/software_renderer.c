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
    
    // objects part 1: load some teddybears from object file 
    for (uint32_t i = 0; i < 2; i++) {
        zpolygons_to_render[i] =
            load_from_obj_file("teddybear.obj");
        zpolygons_to_render_size += 1;
        
        float base_x = i % 3 == 1 ? 0.0f : -30.0f; 
        float base_y = i % 2 == 1 ? 0.0f : -5.0f;
        zpolygons_to_render[i]->x = base_x + (i * 7.0f);
        zpolygons_to_render[i]->y = base_y + (i * 7.0f);
        zpolygons_to_render[i]->z = (25.0f + (i * 35.0f));
    }

    // objects 2: load some hard-coded cubes
    for (uint32_t i = 2; i < 3; i++) {
        zpolygons_to_render[i] = get_box();
        zpolygons_to_render_size += 1;
    }
    
    // initialize global zLightSource objects, to set up
    // our lighting for the scene
    zlights_to_apply[0].x = 45.0f;
    zlights_to_apply[0].y = 2.5f;
    zlights_to_apply[0].z = 80.0f;
    zlights_to_apply[0].reach = 150.0f;
    zlights_to_apply[0].ambient = 0.1f;
    zlights_to_apply[0].diffuse = 2.0f;
    zlights_to_apply_size = 1;
    
    // add a white cube to represent the light source
    zpolygons_to_render_size += 1;
    uint32_t i = zpolygons_to_render_size - 1;
    zpolygons_to_render[i] = get_box();
    zpolygons_to_render[i]->x = zlights_to_apply[0].x;
    zpolygons_to_render[i]->y = zlights_to_apply[0].y;
    zpolygons_to_render[i]->z = zlights_to_apply[0].z;
    for (
        uint32_t j = 0;
        j < zpolygons_to_render[i]->triangles_size;
        j++)
    {
        // bright white
        zpolygons_to_render[i]->triangles[j].color[0] = 50.0f;
        zpolygons_to_render[i]->triangles[j].color[1] = 50.0f;
        zpolygons_to_render[i]->triangles[j].color[2] = 50.0f;
        zpolygons_to_render[i]->triangles[j].color[3] = 50.0f;
        zpolygons_to_render[i]->triangles[j].texture_i = -1;
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
    // camera.x += 0.01f;
    // camera.y += 0.01f;
    // camera.z += 0.02f;
    
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
    
    for (
        uint32_t i = 0;
        i < (zpolygons_to_render_size - 1);
        i++)
    {
        zpolygons_to_render[i]->x -= 0.001;
        zpolygons_to_render[i]->y += 0.001;
        zpolygons_to_render[i]->z += 0.01;
        zpolygons_to_render[i]->x_angle += 0.04f;
        zpolygons_to_render[i]->y_angle += 0.04f;
    }
    
    // move our light source
    uint32_t i = zpolygons_to_render_size - 1;
    zpolygons_to_render[i]->y -= 0.001;
    if (zpolygons_to_render[i]->z > 6.0f) {
        zpolygons_to_render[i]->z -= 0.15;
        zpolygons_to_render[i]->x -= 0.07;
    } else if (zpolygons_to_render[i]->x > -6.0f) {
        zpolygons_to_render[i]->x -= 0.15;
    }
    zlights_to_apply[0].x = zpolygons_to_render[i]->x;
    zlights_to_apply[0].y = zpolygons_to_render[i]->y;
    zlights_to_apply[0].z = zpolygons_to_render[i]->z;
    
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
        float perspective_dot_product =
            get_visibility_rating(
                camera,
                triangles_to_draw + i,
                0);
        
        if (perspective_dot_product < 0.0f) {
            // colored triangle
            Vertex triangle_to_draw[3];
            
            for (uint32_t l = 0; l < zlights_to_apply_size; l++)
            {
                // add lighting to the 3 vertices
                for (uint32_t m = 0; m < 3; m++) {
                    triangle_to_draw[m].lighting = 0.0f;
                    
                    zVertex light_source;
                    light_source.x = zlights_to_apply[l].x;
                    light_source.y = zlights_to_apply[l].y;
                    light_source.z = zlights_to_apply[l].z;
                   
                    float distance = get_distance_to_ztriangle(
                        light_source,
                        triangles_to_draw[i]);
                    float distance_mod = 1.0f -
                        (distance / zlights_to_apply[l].reach);
                    if (distance_mod < 0.0f) {
                        distance_mod = 0.0f;
                    }
                    assert(distance_mod < 1.01f);
                    
                    // add ambient lighting 
                    triangle_to_draw[m].lighting +=
                        zlights_to_apply[l].ambient
                            * distance_mod;
                    
                    // add diffuse lighting
                    float diffuse_dot = get_visibility_rating(
                        light_source,
                        triangles_to_draw + i,
                        m);
                    
                    // TODO remove assert
                    if (diffuse_dot > 1.02f) {
                        printf("ERROR: diffuse dot was: %f\n",
                            diffuse_dot);
                        assert(0);
                    }
                    
                    if (diffuse_dot < 0.0f)
                    {
                        triangle_to_draw[m].lighting +=
                            (diffuse_dot
                                * -1
                                * zlights_to_apply[l].diffuse);
                    }
                }
            }
            
            // TODO: set uv coordinates for texture mapping
            // properly
            // this is just a hack for testing
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

