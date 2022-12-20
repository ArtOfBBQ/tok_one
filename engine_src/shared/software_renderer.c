#include "software_renderer.h"

static uint32_t renderer_initialized = false;

// all of these are for vectorized
// manipulation of our triangles vertices
#define VERTICES_CAP 500000
static float * polygons_x; // the xyz of the parent polygons
static float * polygons_y;
static float * polygons_z;
static float * triangle_vertices_x; // the xyz offsets of the triangles that make up the poly
static float * triangle_vertices_y;
static float * triangle_vertices_z;
static float * camera_multipliers;
static float * lighting_multipliers;
static float * working_memory_1;
static float * visibility_ratings;
static Vertex * rendered_vertices;
static int32_t * rendered_triangles_touchable_ids;

void init_renderer() {
    renderer_initialized = true;
    
    camera.x = 0.0f;
    camera.y = 0.0f;
    camera.z = 0.0f;
    camera.x_angle = 0.0f;
    camera.y_angle = 0.0f;
    camera.z_angle = 0.0f;
    
    triangle_vertices_x = (float *)malloc_from_unmanaged_aligned(
        VERTICES_CAP * sizeof(float), 32);
    triangle_vertices_y = (float *)malloc_from_unmanaged_aligned(
        VERTICES_CAP * sizeof(float), 32);
    triangle_vertices_z = (float *)malloc_from_unmanaged_aligned(
        VERTICES_CAP * sizeof(float), 32);
    polygons_x = (float *)malloc_from_unmanaged_aligned(
        VERTICES_CAP * sizeof(float), 32);
    polygons_y = (float *)malloc_from_unmanaged_aligned(
        VERTICES_CAP * sizeof(float), 32);
    polygons_z = (float *)malloc_from_unmanaged_aligned(
        VERTICES_CAP * sizeof(float), 32);
    working_memory_1 = (float *)malloc_from_unmanaged_aligned(
        VERTICES_CAP * sizeof(float), 32);
    camera_multipliers = (float *)malloc_from_unmanaged_aligned(
        VERTICES_CAP * sizeof(float), 32);
    lighting_multipliers = (float *)malloc_from_unmanaged_aligned(
        VERTICES_CAP * sizeof(float), 32);
    //cosines = (float *)malloc_from_unmanaged_aligned(
    //    VERTICES_CAP * sizeof(float), 32);
    //sines = (float *)malloc_from_unmanaged_aligned(
    //    VERTICES_CAP * sizeof(float), 32);
    visibility_ratings = (float *)malloc_from_unmanaged_aligned(
        VERTICES_CAP * sizeof(float), 32);
    rendered_vertices = (Vertex *)malloc_from_unmanaged_aligned(
        VERTICES_CAP * sizeof(Vertex), 32);
    rendered_triangles_touchable_ids = (int32_t *)malloc_from_unmanaged_aligned(
        VERTICES_CAP * sizeof(int32_t) / 3, 32);
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
    
    if (zpolygons_to_render_size == 0 && texquads_to_render_size == 0) {
        return;
    }
    
    assert(zpolygons_to_render_size < ZPOLYGONS_TO_RENDER_ARRAYSIZE);
    
    uint32_t all_triangles_size = 0;
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
            all_triangles_size++;
        }
    }
    all_triangles_size += (texquads_to_render_size * 2);
    
    if (all_triangles_size == 0) { return; }
    
    // transform all triangle vertices
    // there's actually only 1 angle per polygon, so
    // this is very wasteful
    float x_angles[(all_triangles_size * 3) + (SIMD_FLOAT_WIDTH-1)];
    float y_angles[(all_triangles_size * 3) + (SIMD_FLOAT_WIDTH-1)];
    float z_angles[(all_triangles_size * 3) + (SIMD_FLOAT_WIDTH-1)];
    
    const uint32_t vertices_size = all_triangles_size * 3;
    assert(vertices_size < VERTICES_CAP);
    
    uint32_t cur_vertex = 0;
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
            for (uint32_t m = 0; m < 3; m++) {
                triangle_vertices_x[cur_vertex] =
                    zpolygons_to_render[i].triangles[j].vertices[m].x;
                triangle_vertices_y[cur_vertex] =
                    zpolygons_to_render[i].triangles[j].vertices[m].y;
                triangle_vertices_z[cur_vertex] =
                    zpolygons_to_render[i].triangles[j].vertices[m].z;
                polygons_x[cur_vertex] = zpolygons_to_render[i].x;
                polygons_y[cur_vertex] = zpolygons_to_render[i].y;
                polygons_z[cur_vertex] = zpolygons_to_render[i].z;
                x_angles[cur_vertex] = zpolygons_to_render[i].x_angle;
                y_angles[cur_vertex] = zpolygons_to_render[i].y_angle;
                z_angles[cur_vertex] = zpolygons_to_render[i].z_angle;
                
                working_memory_1[cur_vertex] = zpolygons_to_render[i].scale_factor;
                
                camera_multipliers[cur_vertex] = (1.0f * !zpolygons_to_render[i].ignore_camera);
                // lighting_multipliers[cur_vertex] = (1.0f * !zpolygons_to_render[i].ignore_lighting);
                cur_vertex += 1;
            }
        }
    }
    
    for (uint32_t i = 0; i < texquads_to_render_size; i++) {
        float offset_left =
            texquads_to_render[i].left_x + texquads_to_render[i].x_offset;
        float offset_right = offset_left + texquads_to_render[i].width;
        float offset_mid_x = (offset_left + offset_right) / 2;
        
        float offset_top = texquads_to_render[i].top_y + texquads_to_render[i].y_offset;
        float offset_bottom = offset_top - texquads_to_render[i].height;
        float offset_mid_y = (offset_top + offset_bottom) / 2;
        
        // *** top left triangle
        // topleft vertex
        triangle_vertices_x[cur_vertex    ] = offset_left - offset_mid_x;
        triangle_vertices_y[cur_vertex    ] = offset_top - offset_mid_y;
        triangle_vertices_z[cur_vertex    ] = 0;
        // topright vertex
        triangle_vertices_x[cur_vertex + 1] = offset_right - offset_mid_x;
        triangle_vertices_y[cur_vertex + 1] = offset_top - offset_mid_y;
        triangle_vertices_z[cur_vertex + 1] = 0;
        // bottomleft vertex
        triangle_vertices_x[cur_vertex + 2] = offset_left - offset_mid_x;
        triangle_vertices_y[cur_vertex + 2] = offset_bottom - offset_mid_y;
        triangle_vertices_z[cur_vertex + 2] = 0;
        
        // *** bottom right triangle
        // topright vertex
        triangle_vertices_x[cur_vertex + 3] = offset_right - offset_mid_x;
        triangle_vertices_y[cur_vertex + 3] = offset_top - offset_mid_y;
        triangle_vertices_z[cur_vertex + 3] = 0;
        // bottomright vertex
        triangle_vertices_x[cur_vertex + 4] = offset_right - offset_mid_x;
        triangle_vertices_y[cur_vertex + 4] = offset_bottom - offset_mid_y;
        triangle_vertices_z[cur_vertex + 4] = 0;
        // bottomleft vertex
        triangle_vertices_x[cur_vertex + 5] = offset_left - offset_mid_x;
        triangle_vertices_y[cur_vertex + 5] = offset_bottom - offset_mid_y;
        triangle_vertices_z[cur_vertex + 5] = 0;
        
        polygons_x[cur_vertex  ] = offset_mid_x;
        polygons_y[cur_vertex  ] = offset_mid_y;
        polygons_z[cur_vertex  ] = texquads_to_render[i].z;
        polygons_x[cur_vertex+1] = offset_mid_x;
        polygons_y[cur_vertex+1] = offset_mid_y;
        polygons_z[cur_vertex+1] = texquads_to_render[i].z;
        polygons_x[cur_vertex+2] = offset_mid_x;
        polygons_y[cur_vertex+2] = offset_mid_y;
        polygons_z[cur_vertex+2] = texquads_to_render[i].z;
        polygons_x[cur_vertex+3] = offset_mid_x;
        polygons_y[cur_vertex+3] = offset_mid_y;
        polygons_z[cur_vertex+3] = texquads_to_render[i].z;
        polygons_x[cur_vertex+4] = offset_mid_x;
        polygons_y[cur_vertex+4] = offset_mid_y;
        polygons_z[cur_vertex+4] = texquads_to_render[i].z;
        polygons_x[cur_vertex+5] = offset_mid_x;
        polygons_y[cur_vertex+5] = offset_mid_y;
        polygons_z[cur_vertex+5] = texquads_to_render[i].z;
        
        x_angles[cur_vertex  ] = 0.0f;
        y_angles[cur_vertex  ] = 0.0f;
        z_angles[cur_vertex  ] = texquads_to_render[i].z_angle;
        x_angles[cur_vertex+1] = 0.0f;
        y_angles[cur_vertex+1] = 0.0f;
        z_angles[cur_vertex+1] = texquads_to_render[i].z_angle;
        x_angles[cur_vertex+2] = 0.0f;
        y_angles[cur_vertex+2] = 0.0f;
        z_angles[cur_vertex+2] = texquads_to_render[i].z_angle;
        x_angles[cur_vertex+3] = 0.0f;
        y_angles[cur_vertex+3] = 0.0f;
        z_angles[cur_vertex+3] = texquads_to_render[i].z_angle;
        x_angles[cur_vertex+4] = 0.0f;
        y_angles[cur_vertex+4] = 0.0f;
        z_angles[cur_vertex+4] = texquads_to_render[i].z_angle;
        x_angles[cur_vertex+5] = 0.0f;
        y_angles[cur_vertex+5] = 0.0f;
        z_angles[cur_vertex+5] = texquads_to_render[i].z_angle;
        
        working_memory_1[cur_vertex    ] = texquads_to_render[i].scale_factor;
        working_memory_1[cur_vertex + 1] = texquads_to_render[i].scale_factor;
        working_memory_1[cur_vertex + 2] = texquads_to_render[i].scale_factor;
        working_memory_1[cur_vertex + 3] = texquads_to_render[i].scale_factor;
        working_memory_1[cur_vertex + 4] = texquads_to_render[i].scale_factor;
        working_memory_1[cur_vertex + 5] = texquads_to_render[i].scale_factor;
        
        camera_multipliers[cur_vertex    ] =
            (1.0f * !texquads_to_render[i].ignore_camera);
        camera_multipliers[cur_vertex + 1] =
            (1.0f * !texquads_to_render[i].ignore_camera);
        camera_multipliers[cur_vertex + 2] =
            (1.0f * !texquads_to_render[i].ignore_camera);
        camera_multipliers[cur_vertex + 3] =
            (1.0f * !texquads_to_render[i].ignore_camera);
        camera_multipliers[cur_vertex + 4] =
            (1.0f * !texquads_to_render[i].ignore_camera);
        camera_multipliers[cur_vertex + 5] =
            (1.0f * !texquads_to_render[i].ignore_camera);
        
        cur_vertex += 6;
    }
    
    float cos_camera_x_angles[SIMD_FLOAT_WIDTH];
    float sin_camera_x_angles[SIMD_FLOAT_WIDTH];
    for (uint32_t i = 0; i < SIMD_FLOAT_WIDTH; i++) {
        cos_camera_x_angles[i] = cosf(camera.x_angle);
        sin_camera_x_angles[i] = sinf(camera.x_angle);
    }
    SIMD_FLOAT simd_cos_camera_x_angle = simd_load_floats(cos_camera_x_angles);
    SIMD_FLOAT simd_sin_camera_x_angle = simd_load_floats(sin_camera_x_angles);
    
    float cos_camera_y_angles[SIMD_FLOAT_WIDTH];
    float sin_camera_y_angles[SIMD_FLOAT_WIDTH];
    for (uint32_t i = 0; i < SIMD_FLOAT_WIDTH; i++) {
        cos_camera_y_angles[i] = cosf(camera.y_angle);
        sin_camera_y_angles[i] = sinf(camera.y_angle);
    }
    SIMD_FLOAT simd_cos_camera_y_angle = simd_load_floats(cos_camera_y_angles);
    SIMD_FLOAT simd_sin_camera_y_angle = simd_load_floats(sin_camera_y_angles);
    
    float cos_camera_z_angles[SIMD_FLOAT_WIDTH];
    float sin_camera_z_angles[SIMD_FLOAT_WIDTH];
    for (uint32_t i = 0; i < SIMD_FLOAT_WIDTH; i++) {
        cos_camera_z_angles[i] = cosf(camera.z_angle);
        sin_camera_z_angles[i] = sinf(camera.z_angle);
    }
    SIMD_FLOAT simd_cos_camera_z_angle = simd_load_floats(cos_camera_z_angles);
    SIMD_FLOAT simd_sin_camera_z_angle = simd_load_floats(sin_camera_z_angles);
    
    for (uint32_t i = 0; i < vertices_size; i += SIMD_FLOAT_WIDTH) {
        // apply scaling modifier
        SIMD_FLOAT scaling_modifiers = simd_load_floats(working_memory_1 + i);
        
        SIMD_FLOAT simd_vertices_x = simd_load_floats(triangle_vertices_x + i);
        simd_vertices_x = simd_mul_floats(simd_vertices_x, scaling_modifiers);
        
        SIMD_FLOAT simd_vertices_y = simd_load_floats(triangle_vertices_y + i);
        simd_vertices_y = simd_mul_floats(simd_vertices_y, scaling_modifiers);
        
        SIMD_FLOAT simd_vertices_z = simd_load_floats(triangle_vertices_z + i);
        simd_vertices_z = simd_mul_floats(simd_vertices_z, scaling_modifiers);
        
        // rotate vertices
        float cosines[SIMD_FLOAT_WIDTH];
        float sines[SIMD_FLOAT_WIDTH];
        for (uint32_t j = 0; j < SIMD_FLOAT_WIDTH; j++) {
            cosines[j] = cosf(x_angles[i + j]);
            sines[j] = sinf(x_angles[i + j]);
        }
        SIMD_FLOAT simd_cosines = simd_load_floats(cosines);
        SIMD_FLOAT simd_sines = simd_load_floats(sines);
        
        x_rotate_zvertices_inplace(
           /* SIMD_FLOAT * vec_to_rotate_y      : */
               &simd_vertices_y,
           /* SIMD_FLOAT * vec_to_rotate_z      : */
               &simd_vertices_z,
           /* const SIMD_FLOAT cos_angles       : */
               simd_cosines,
           /* const SIMD_FLOAT sin_angles       : */
               simd_sines);
        
        for (uint32_t j = 0; j < SIMD_FLOAT_WIDTH; j++) {
            cosines[j] = cosf(y_angles[i + j]);
            sines[j] = sinf(y_angles[i + j]);
        }
        simd_cosines = simd_load_floats(cosines);
        simd_sines = simd_load_floats(sines);
        y_rotate_zvertices_inplace(
           /* SIMD_FLOAT * vec_to_rotate_x      : */
               &simd_vertices_x,
           /* SIMD_FLOAT * vec_to_rotate_z      : */
               &simd_vertices_z,
           /* const SIMD_FLOAT cos_angles       : */
               simd_cosines,
           /* const SIMD_FLOAT sin_angles       : */
               simd_sines);
        
        for (uint32_t j = 0; j < SIMD_FLOAT_WIDTH; j++) {
            cosines[j] = cosf(z_angles[i + j]);
            sines[j] = sinf(z_angles[i + j]);
        }
        simd_cosines = simd_load_floats(cosines);
        simd_sines = simd_load_floats(sines);
        z_rotate_zvertices_inplace(
           /* SIMD_FLOAT * vec_to_rotate_x      : */
               &simd_vertices_x,
           /* SIMD_FLOAT * vec_to_rotate_y      : */
               &simd_vertices_y,
           /* const SIMD_FLOAT cos_angles       : */
               simd_cosines,
           /* const SIMD_FLOAT sin_angles       : */
               simd_sines);
        
        SIMD_FLOAT simd_polygons_x = simd_load_floats(polygons_x + i);
        SIMD_FLOAT simd_polygons_y = simd_load_floats(polygons_y + i);
        SIMD_FLOAT simd_polygons_z = simd_load_floats(polygons_z + i);
        simd_vertices_x = simd_add_floats(simd_vertices_x, simd_polygons_x);
        simd_vertices_y = simd_add_floats(simd_vertices_y, simd_polygons_y);
        simd_vertices_z = simd_add_floats(simd_vertices_z, simd_polygons_z);
        
        // translate the world so that the camera becomes 0,0,0
        float camera_pos[SIMD_FLOAT_WIDTH];
        for (uint32_t _ = 0; _ < SIMD_FLOAT_WIDTH; _++) {
            camera_pos[_] = camera.x;
        }
        SIMD_FLOAT simd_camera_mod = simd_load_floats(camera_pos);
        SIMD_FLOAT simd_camera_factors = simd_load_floats(camera_multipliers + i); 
        simd_camera_mod = simd_mul_floats(simd_camera_mod, simd_camera_factors);
        simd_vertices_x = simd_sub_floats(simd_vertices_x, simd_camera_mod);
        
        for (uint32_t _ = 0; _ < SIMD_FLOAT_WIDTH; _++) {
            camera_pos[_] = camera.y;
        }
        simd_camera_mod = simd_load_floats(camera_pos);
        simd_camera_mod = simd_mul_floats(simd_camera_mod, simd_camera_factors);
        simd_vertices_y = simd_sub_floats(simd_vertices_y, simd_camera_mod);
        
        for (uint32_t _ = 0; _ < SIMD_FLOAT_WIDTH; _++) {
            camera_pos[_] = camera.z;
        }
        simd_camera_mod = simd_load_floats(camera_pos);
        simd_camera_mod = simd_mul_floats(simd_camera_mod, simd_camera_factors);
        simd_vertices_z = simd_sub_floats(simd_vertices_z, simd_camera_mod);
        
        
        SIMD_FLOAT modified_cos_angle = simd_mul_floats(
            simd_cos_camera_x_angle,
            simd_camera_factors);
        SIMD_FLOAT modified_sin_angle = simd_mul_floats(
            simd_sin_camera_x_angle,
            simd_camera_factors);
        x_rotate_zvertices_inplace(
           /* SIMD_FLOAT * vec_to_rotate_y      : */
               &simd_vertices_y,
           /* SIMD_FLOAT * vec_to_rotate_z      : */
               &simd_vertices_z,
           /* const SIMD_FLOAT cos_angles       : */
               modified_cos_angle,
           /* const SIMD_FLOAT sin_angles       : */
               modified_sin_angle);
        
        modified_cos_angle = simd_mul_floats(
            simd_cos_camera_y_angle,
            simd_camera_factors);
        modified_sin_angle = simd_mul_floats(
            simd_sin_camera_y_angle,
            simd_camera_factors);
        y_rotate_zvertices_inplace(
           /* SIMD_FLOAT * vec_to_rotate_x      : */
               &simd_vertices_x,
           /* SIMD_FLOAT * vec_to_rotate_z      : */
               &simd_vertices_z,
           /* const SIMD_FLOAT cos_angles       : */
               modified_cos_angle,
           /* const SIMD_FLOAT sin_angles       : */
               modified_sin_angle);
        
        modified_cos_angle = simd_mul_floats(
            simd_cos_camera_z_angle,
            simd_camera_factors);
        modified_sin_angle = simd_mul_floats(
            simd_sin_camera_z_angle,
            simd_camera_factors);
        z_rotate_zvertices_inplace(
           /* SIMD_FLOAT * vec_to_rotate_x      : */
               &simd_vertices_x,
           /* SIMD_FLOAT * vec_to_rotate_y      : */
               &simd_vertices_y,
           /* const SIMD_FLOAT cos_angles       : */
               modified_cos_angle,
           /* const SIMD_FLOAT sin_angles       : */
               modified_sin_angle);
        
        simd_store_floats(triangle_vertices_x + i, simd_vertices_x);
        simd_store_floats(triangle_vertices_y + i, simd_vertices_y);
        simd_store_floats(triangle_vertices_z + i, simd_vertices_z);
    }
    
    // we're not using the camera because the entire world
    // was translated to have the camera be at 0,0,0
    zVertex origin;
    origin.x = 0.0f;
    origin.y = 0.0f;
    origin.z = 0.0f;
    
    // this gets 1 'visibility rating' (a dot product) per vertex
    get_visibility_ratings(
        origin,
        triangle_vertices_x,
        triangle_vertices_y,
        triangle_vertices_z,
        vertices_size,
        visibility_ratings);
    
    // Next, we'll do a bunch of copying of the visible triangles to the front
    // of the arrays, so we can ignore the invisible triangles at the end
    uint32_t visible_triangle_i = 0;
    uint32_t all_triangle_i = 0;
    for (
        uint32_t zp_i = 0;
        zp_i < zpolygons_to_render_size;
        zp_i++)
    {
        if (zpolygons_to_render[zp_i].deleted) {
            all_triangle_i += zpolygons_to_render[zp_i].triangles_size;
            continue;
        }
        
        for (
           int32_t i = 0;
           i < (int32_t)zpolygons_to_render[zp_i].triangles_size;
           i++)
        {
            uint32_t first_visible_vertex_i = visible_triangle_i * 3;
            uint32_t first_all_vertex_i = all_triangle_i * 3;
            
            log_assert(first_visible_vertex_i < vertices_size);
            if (
                (visibility_ratings[first_all_vertex_i] < 0.0f) &&
                (triangle_vertices_z[first_all_vertex_i  ] > projection_constants.near ||
                 triangle_vertices_z[first_all_vertex_i+1] > projection_constants.near ||
                 triangle_vertices_z[first_all_vertex_i+2] > projection_constants.near))
            {
                for (uint32_t m = 0; m < 3; m++) {
                    uint32_t visible_vertices_i = first_visible_vertex_i + m;
                    uint32_t all_vertices_i = first_all_vertex_i + m;
                    
                    triangle_vertices_x[visible_vertices_i] =
                        triangle_vertices_x[all_vertices_i];
                    triangle_vertices_y[visible_vertices_i] =
                        triangle_vertices_y[all_vertices_i];
                    triangle_vertices_z[visible_vertices_i] =
                        triangle_vertices_z[all_vertices_i];
                    
                    lighting_multipliers[visible_vertices_i] =
                        (1.0f * !zpolygons_to_render[zp_i].ignore_lighting);
                    rendered_vertices[visible_vertices_i].lighting[0] =
                        (lighting_multipliers[visible_vertices_i] - 1.0f) * -1.0f;
                    rendered_vertices[visible_vertices_i].lighting[1] = 
                        (lighting_multipliers[visible_vertices_i] - 1.0f) * -1.0f;
                    rendered_vertices[visible_vertices_i].lighting[2] =
                        (lighting_multipliers[visible_vertices_i] - 1.0f) * -1.0f;
                    rendered_vertices[visible_vertices_i].lighting[3] = 1.0f;
                    
                    rendered_vertices[visible_vertices_i].texture_i =
                        zpolygons_to_render[zp_i].triangles[i].texture_i;
                    rendered_vertices[visible_vertices_i].texturearray_i =
                        zpolygons_to_render[zp_i].triangles[i].texturearray_i;
                    
                    rendered_vertices[visible_vertices_i].uv[0] =
                        zpolygons_to_render[zp_i].triangles[i].vertices[m].uv[0];
                    rendered_vertices[visible_vertices_i].uv[1] =
                        zpolygons_to_render[zp_i].triangles[i].vertices[m].uv[1];
                    
                    rendered_vertices[visible_vertices_i].RGBA[0] =
                        zpolygons_to_render[zp_i].triangles[i].color[0];
                    rendered_vertices[visible_vertices_i].RGBA[1] =
                        zpolygons_to_render[zp_i].triangles[i].color[1];
                    rendered_vertices[visible_vertices_i].RGBA[2] =
                        zpolygons_to_render[zp_i].triangles[i].color[2];
                    rendered_vertices[visible_vertices_i].RGBA[3] =
                        zpolygons_to_render[zp_i].triangles[i].color[3];
                    
                    rendered_triangles_touchable_ids[visible_triangle_i] =
                        zpolygons_to_render[zp_i].touchable_id;
                }
                
                visible_triangle_i += 1;
            }
            
            all_triangle_i += 1;
        }
    }
    
    float left_uv_coord = 0.0f;
    float right_uv_coord = 1.0f;
    float bottom_uv_coord = 1.0f;
    float top_uv_coord = 0.0f;
    for (
       int32_t tq_i = 0;
       tq_i < texquads_to_render_size;
       tq_i++)
    {
        uint32_t first_visible_vertex_i = visible_triangle_i * 3;
        uint32_t first_all_vertex_i = all_triangle_i * 3;
        
        log_assert(first_visible_vertex_i < vertices_size);
        if (visibility_ratings[first_all_vertex_i] < 0.0f)
        {
            for (uint32_t m = 0; m < 6; m++) {
                triangle_vertices_x[first_visible_vertex_i + m] =
                    triangle_vertices_x[first_all_vertex_i + m];
                triangle_vertices_y[first_visible_vertex_i + m] =
                    triangle_vertices_y[first_all_vertex_i + m];
                triangle_vertices_z[first_visible_vertex_i + m] =
                    triangle_vertices_z[first_all_vertex_i + m];
                
                lighting_multipliers[first_visible_vertex_i + m] =
                    (1.0f * !texquads_to_render[tq_i].ignore_lighting);
                
                rendered_vertices[first_visible_vertex_i + m].lighting[0] =
                    (lighting_multipliers[first_visible_vertex_i + m] - 1.0f) * -1.0f;
                rendered_vertices[first_visible_vertex_i + m].lighting[1] =
                    (lighting_multipliers[first_visible_vertex_i + m] - 1.0f) * -1.0f;
                rendered_vertices[first_visible_vertex_i + m].lighting[2] =
                    (lighting_multipliers[first_visible_vertex_i + m] - 1.0f) * -1.0f;
                rendered_vertices[first_visible_vertex_i + m].lighting[3] = 1.0f;
                
                rendered_vertices[first_visible_vertex_i + m].texture_i =
                    texquads_to_render[tq_i].texture_i;
                rendered_vertices[first_visible_vertex_i + m].texturearray_i =
                    texquads_to_render[tq_i].texturearray_i;
                
                rendered_vertices[first_visible_vertex_i + m].RGBA[0] =
                    texquads_to_render[tq_i].RGBA[0];
                rendered_vertices[first_visible_vertex_i + m].RGBA[1] =
                    texquads_to_render[tq_i].RGBA[1];
                rendered_vertices[first_visible_vertex_i + m].RGBA[2] =
                    texquads_to_render[tq_i].RGBA[2];
                rendered_vertices[first_visible_vertex_i + m].RGBA[3] =
                    texquads_to_render[tq_i].RGBA[3];
                
                rendered_triangles_touchable_ids[visible_triangle_i    ] =
                    texquads_to_render[tq_i].touchable_id;
                rendered_triangles_touchable_ids[visible_triangle_i + 1] =
                    texquads_to_render[tq_i].touchable_id;
            }
            
            // topleft
            rendered_vertices[first_visible_vertex_i + 0].uv[0] = left_uv_coord;
            rendered_vertices[first_visible_vertex_i + 0].uv[1] = top_uv_coord;
            // topright
            rendered_vertices[first_visible_vertex_i + 1].uv[0] = right_uv_coord;
            rendered_vertices[first_visible_vertex_i + 1].uv[1] = top_uv_coord;
            // bottomleft
            rendered_vertices[first_visible_vertex_i + 2].uv[0] = left_uv_coord;
            rendered_vertices[first_visible_vertex_i + 2].uv[1] = bottom_uv_coord;
            // topright
            rendered_vertices[first_visible_vertex_i + 3].uv[0] = right_uv_coord;
            rendered_vertices[first_visible_vertex_i + 3].uv[1] = top_uv_coord;
            // bottomright
            rendered_vertices[first_visible_vertex_i + 4].uv[0] = right_uv_coord;
            rendered_vertices[first_visible_vertex_i + 4].uv[1] = bottom_uv_coord;
            // bottomleft
            rendered_vertices[first_visible_vertex_i + 5].uv[0] = left_uv_coord;
            rendered_vertices[first_visible_vertex_i + 5].uv[1] = bottom_uv_coord;
            
            visible_triangle_i += 2;
        }
        
        all_triangle_i += 2;
    }
    
    uint32_t visible_triangles_size = visible_triangle_i;
    uint32_t visible_vertices_size = visible_triangle_i * 3;
    
    for (
        uint32_t light_i = 0;
        light_i < zlights_transformed_size;
        light_i++)
    {
        ztriangles_apply_lighting(
            triangle_vertices_x,
            triangle_vertices_y,
            triangle_vertices_z,
            lighting_multipliers,
            visible_vertices_size,
            rendered_vertices,
            visible_vertices_size,
            &zlights_transformed[light_i]);
    }
    
    // w is basically just z before projection
    for (uint32_t i = 0; i < visible_triangles_size; i++) {
        rendered_vertices[(i*3)+0].w = triangle_vertices_z[(i*3)+0];
        rendered_vertices[(i*3)+1].w = triangle_vertices_z[(i*3)+1];
        rendered_vertices[(i*3)+2].w = triangle_vertices_z[(i*3)+2]; 
    }
    
    ztriangles_to_2d_inplace(
        triangle_vertices_x,
        triangle_vertices_y,
        triangle_vertices_z,
        visible_vertices_size);
    
    for (uint32_t i = 0; i < visible_triangles_size; i++) {
        // note: this won't overwrite the lighting properties in
        // rendered_vertices, only the positions
        rendered_vertices[(i*3)+0].x = triangle_vertices_x[(i*3)+0];
        rendered_vertices[(i*3)+1].x = triangle_vertices_x[(i*3)+1];
        rendered_vertices[(i*3)+2].x = triangle_vertices_x[(i*3)+2];        
        rendered_vertices[(i*3)+0].y = triangle_vertices_y[(i*3)+0];
        rendered_vertices[(i*3)+1].y = triangle_vertices_y[(i*3)+1];
        rendered_vertices[(i*3)+2].y = triangle_vertices_y[(i*3)+2]; 
        rendered_vertices[(i*3)+0].z = triangle_vertices_z[(i*3)+0];
        rendered_vertices[(i*3)+1].z = triangle_vertices_z[(i*3)+1];
        rendered_vertices[(i*3)+2].z = triangle_vertices_z[(i*3)+2]; 
        
        draw_triangle(
            /* vertices_recipient: */
                next_gpu_workload,
            /* vertex_count_recipient: */
                next_workload_size,
            /* input: */
                rendered_vertices + (i * 3),
            /* touchable_id: */
                rendered_triangles_touchable_ids[i]);
    }    
}
