#include "software_renderer.h"

static uint32_t renderer_initialized = false;

void init_renderer() {
    renderer_initialized = true;
    
    camera.x = 0.0f;
    camera.y = 0.0f;
}

void software_render(
    Vertex * next_gpu_workload,
    uint32_t * next_workload_size,
    uint64_t elapsed_nanoseconds)
{
    (void)elapsed_nanoseconds;
    
    if (renderer_initialized != true) {
        log_append("renderer not initialized, aborting...\n");
        return;
    }
    
    if (
        next_gpu_workload == NULL ||
        next_workload_size == NULL)
    {
        log_append("ERROR: platform layer didnt pass recipients\n");
        return;
    }
    
    if (zpolygons_to_render_size == 0) {
        return;
    }
    
    assert(zpolygons_to_render_size < ZPOLYGONS_TO_RENDER_ARRAYSIZE);
    
    uint32_t triangles_to_draw_size = 0;
    for (
        uint32_t i = 0;
        i < zpolygons_to_render_size;
        i++)
    {
        for (
            uint32_t j = 0;
            j < zpolygons_to_render[i].triangles_size;
            j++)
        {
            triangles_to_draw_size++;
        }
    }
    
    if (triangles_to_draw_size == 0) { return; }
    
    // transform all triangles
    zTriangle triangles_to_draw[triangles_to_draw_size];
    zTriangle visible_triangles[triangles_to_draw_size];
    uint32_t visible_triangles_size = 0;
    zTriangle position_translated;
    zTriangle camera_y_rotated;
    zTriangle camera_x_rotated;
    zTriangle camera_z_rotated;
    zTriangle x_rotated;
    zTriangle y_rotated;
    zTriangle z_rotated;
    uint32_t t = 0;
    for (
        uint32_t i = 0;
        i < zpolygons_to_render_size;
        i++)
    {
        for (
            uint32_t j = 0;
            j < zpolygons_to_render[i].triangles_size;
            j++)
        {
            log_assert(t < triangles_to_draw_size);
            
            x_rotated = x_rotate_ztriangle(
                zpolygons_to_render[i].triangles + j,
                zpolygons_to_render[i].x_angle);
            y_rotated = y_rotate_ztriangle(
                &x_rotated,
                zpolygons_to_render[i].y_angle);
            z_rotated = z_rotate_ztriangle(
                &y_rotated,
                zpolygons_to_render[i].z_angle);
            
            position_translated = translate_ztriangle(
                /* input: */
                    &z_rotated,
                /* by_x: */
                    zpolygons_to_render[i].x - camera.x,
                /* by_y: */
                    zpolygons_to_render[i].y - camera.y,
                /* by_z: */
                    zpolygons_to_render[i].z - camera.z);
            
            camera_y_rotated = y_rotate_ztriangle(
                &position_translated,
                -camera.y_angle);
            camera_x_rotated = x_rotate_ztriangle(
                &camera_y_rotated,
                -camera.x_angle);
            camera_z_rotated = z_rotate_ztriangle(
                &camera_x_rotated,
                -camera.z_angle);
            
            triangles_to_draw[t] = camera_z_rotated;
            t++;
        }
    }
    
    // we're not using the camera because the entire world
    // was translated to have the camera be at 0,0,0
    zVertex origin;
    origin.x = 0.0f;
    origin.y = 0.0f;
    origin.z = 0.0f; 
    for (
        int32_t i = (int32_t)triangles_to_draw_size - 1;
        i >= 0;
        i -= 1)
    {
        float perspective_dot_product =
            get_visibility_rating(
                origin,
                triangles_to_draw + i);
        
        if (perspective_dot_product < 0.0f
            && triangles_to_draw[i].vertices[0].z
                > projection_constants.near
            && triangles_to_draw[i].vertices[1].z
                > projection_constants.near
            && triangles_to_draw[i].vertices[2].z
                > projection_constants.near)
        {            
            visible_triangles[visible_triangles_size++] = triangles_to_draw[i];
        }
    }
    
    uint32_t rendered_vertices_size = visible_triangles_size * 3;
    Vertex rendered_vertices[rendered_vertices_size];
    for (uint32_t rv_i = 0; rv_i < rendered_vertices_size; rv_i++) {
        rendered_vertices[rv_i].lighting[0] = 0.0f;
        rendered_vertices[rv_i].lighting[1] = 0.0f;
        rendered_vertices[rv_i].lighting[2] = 0.0f;
        rendered_vertices[rv_i].lighting[3] = 1.0f;
    }
    
    for (uint32_t light_i = 0; light_i < zlights_to_apply_size; light_i++) {
        ztriangles_apply_lighting(
            /* zTriangle * inputs: */
                visible_triangles,
            /* const uint32_t inputs_size: */
              visible_triangles_size,
            /* Vertex * recipients: */
              rendered_vertices,
            /* const uint32_t recipients_size: */
              rendered_vertices_size,
            /* zLightSource * zlight_source: */
              &zlights_to_apply[light_i]);
    }
    
    for (uint32_t i = 0; i < visible_triangles_size; i++) {
        // note: this won't overwrite the lighting properties in rendered_vertices,
        // only the positions
        ztriangle_to_2d(
            /* recipient: */
                rendered_vertices + (i * 3),
            /* input: */
                visible_triangles + i);
        
        draw_triangle(
            /* vertices_recipient: */
                next_gpu_workload,
            /* vertex_count_recipient: */
                next_workload_size,
            /* input: */
                rendered_vertices + (i * 3));
    }
    
    if (!application_running) { return; }
}

