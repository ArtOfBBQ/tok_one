#include "software_renderer.h"

void init_renderer() {
    client_logic_startup();
    renderer_initialized = true;
}

void free_renderer() {
    for (
        uint32_t i = 0;
        i < zpolygons_to_render_size;
        i++)
    {
        free_zpolygon(zpolygons_to_render[i]);
    }
}

void software_render(
    Vertex * next_gpu_workload,
    uint32_t * next_workload_size)
{
    if (renderer_initialized != true) {
        printf("renderer not initialized, aborting...\n");
        return;
    }

    client_logic_update();
    
    uint64_t elapsed_since_previous_frame =
        platform_end_timer_get_nanosecs();
    platform_start_timer();
    uint64_t fps = 1000000000 / elapsed_since_previous_frame;
    // printf("fps: %llu\n", fps);
    
    if (
        next_gpu_workload == NULL
        || next_workload_size == NULL)
    {
        printf("ERROR: platform layer didnt pass recipients\n");
        return;
    }
    
    if (zpolygons_to_render_size == 0) {
        printf("there's nothing to render - returning...\n");
        return;
    }
    
    // animate our objects
    for (
        uint32_t i = 0;
        i < (zpolygons_to_render_size);
        i++)
    {
        zpolygons_to_render[i]->x -= 0.005f;
        zpolygons_to_render[i]->z_angle += 0.03f;
        zpolygons_to_render[i]->x_angle += 0.021f;
        zpolygons_to_render[i]->y_angle += 0.015f;
    }
    
    // move our light source
    uint32_t light_i = zpolygons_to_render_size - 1;
    zpolygons_to_render[light_i]->y -= 0.001;
    if (
        zpolygons_to_render[light_i]->z > 20.0f)
    {
        zpolygons_to_render[light_i]->z -= 1.2;
        zpolygons_to_render[light_i]->x -= 0.14;
    }
    zlights_to_apply[0].x = zpolygons_to_render[light_i]->x;
    zlights_to_apply[0].y = zpolygons_to_render[light_i]->y;
    zlights_to_apply[0].z = zpolygons_to_render[light_i]->z;
    
    uint32_t triangles_to_render = 0;
    for (
        uint32_t i = 0;
        i < zpolygons_to_render_size;
        i++)
    {
        for (
            uint32_t j = 0;
            j < zpolygons_to_render[i]->triangles_size;
            j++)
        {
            triangles_to_render++;
        }
    }
    
    if (triangles_to_render == 0) { return; }
    
    // transform all triangles
    zTriangle triangles_to_draw[triangles_to_render];
    zTriangle position_translated;
    zTriangle camera_y_rotated;
    zTriangle camera_x_rotated;
    zTriangle camera_z_rotated;
    zTriangle camera_translated;
    zTriangle x_rotated;
    zTriangle y_rotated;
    zTriangle z_rotated;
    minimaps_clear();
    uint32_t t = 0;
    for (
        uint32_t i = 0;
        i < zpolygons_to_render_size;
        i++)
    {
        decodedimg_add_zpolygon(
            &minimap2,
            zpolygons_to_render[i]);
        
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
            
            position_translated = translate_ztriangle(
                /* input: */
                    &z_rotated,
                /* x: */
                    zpolygons_to_render[i]->x - camera.x,
                /* y: */
                    zpolygons_to_render[i]->y - camera.y,
                /* z: */
                    zpolygons_to_render[i]->z - camera.z);
            
            camera_y_rotated = y_rotate_triangle(
                &position_translated,
                -camera.y_angle);
            camera_x_rotated = x_rotate_triangle(
                &camera_y_rotated,
                -camera.x_angle);
            camera_z_rotated = z_rotate_triangle(
                &camera_x_rotated,
                -camera.z_angle);
            
            decodedimg_add_triangle(&minimap, &camera_z_rotated);
            triangles_to_draw[t] = camera_z_rotated;
            t++;
        }
    }
    
    // minimap_add_camera(&camera);
    zCamera imaginary_camera_at_origin;
    imaginary_camera_at_origin.x = 0.0f;
    imaginary_camera_at_origin.y = 0.0f;
    imaginary_camera_at_origin.z = 0.0f;
    imaginary_camera_at_origin.x_angle = 0.0f;
    imaginary_camera_at_origin.y_angle = 0.0f;
    imaginary_camera_at_origin.z_angle = 0.0f;
    decodedimg_add_camera(&minimap, &imaginary_camera_at_origin);
    decodedimg_add_camera(&minimap2, &camera);
    
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
        // we're not using the camera because the entire world
        // was translated to have the camera be at 0,0,0
        zVertex origin;
        origin.x = 0.0f;
        origin.y = 0.0f;
        origin.z = 0.0f; 
        float perspective_dot_product =
            get_visibility_rating(
                origin,
                triangles_to_draw + i,
                0);
        
        if (perspective_dot_product < 0.0f
            && triangles_to_draw[i].vertices[0].z
                > projection_constants.near
            && triangles_to_draw[i].vertices[1].z
                > projection_constants.near
            && triangles_to_draw[i].vertices[2].z
                > projection_constants.near)
        {
            Vertex triangle_to_draw[3];
           
            // TODO: translate all lights in advance,
            // instead of repeating the work for every triangle
            // drawn
            for (
                uint32_t l = 0;
                l < zlights_to_apply_size;
                l++)
            {
                zVertex translated_light_pos;
                translated_light_pos.x =
                    zlights_to_apply[l].x - camera.x;
                translated_light_pos.y =
                    zlights_to_apply[l].y - camera.y;
                translated_light_pos.z =
                    zlights_to_apply[l].z - camera.z;
                translated_light_pos = x_rotate_zvertex(
                    &translated_light_pos,
                    -1 * camera.x_angle);
                translated_light_pos = y_rotate_zvertex(
                    &translated_light_pos,
                    -1 * camera.y_angle);
                translated_light_pos = z_rotate_zvertex(
                    &translated_light_pos,
                    -1 * camera.z_angle);
                
                zLightSource translated_light =
                    zlights_to_apply[l];
                translated_light.x = translated_light_pos.x;
                translated_light.y = translated_light_pos.y;
                translated_light.z = translated_light_pos.z;
                
                ztriangle_apply_lighting(
                    /* recipient: */
                        triangle_to_draw,
                    /* input: */
                        triangles_to_draw + i,
                    /* zlight_source: */
                        &translated_light);
            }
            
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

