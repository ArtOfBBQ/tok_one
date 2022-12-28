#include "software_renderer.h"

static uint32_t renderer_initialized = false;

// all of these are for vectorized manipulation of our triangles vertices
// they will all be very small heap arrays, aligned to 32 bytes, in succession
// so hopefully they'll stay in L1 cache
static float * x_angles;
static float * y_angles;
static float * z_angles;
static float * triangle_vertices_x;
static float * triangle_vertices_y;
static float * triangle_vertices_z;
static float * triangle_normals_x;
static float * triangle_normals_y;
static float * triangle_normals_z;
static float * parent_x;
static float * parent_y;
static float * parent_z;
static float * scale_factors;
static float * camera_multipliers;
static float * lighting_multipliers;
static float * texturearray_indexes;
static float * texture_indexes;
static float * rgba_reds;
static float * rgba_greens;
static float * rgba_blues;
static float * rgba_alphas;
static float * u_coords;
static float * v_coords;
static int32_t * touchable_ids;

void init_renderer() {
    renderer_initialized = true;
    
    camera.x = 0.0f;
    camera.y = 0.0f;
    camera.z = 0.0f;
    camera.x_angle = 0.0f;
    camera.y_angle = 0.0f;
    camera.z_angle = 0.0f;
    
    x_angles = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    y_angles = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    z_angles = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    triangle_vertices_x = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    triangle_vertices_y = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    triangle_vertices_z = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    triangle_normals_x = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    triangle_normals_y = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    triangle_normals_z = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    parent_x = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    parent_y = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    parent_z = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    scale_factors = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    camera_multipliers = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    lighting_multipliers = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    touchable_ids = (int32_t *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(int32_t), 32);
    texturearray_indexes = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    texture_indexes = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    rgba_reds = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    rgba_greens = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    rgba_blues = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    rgba_alphas = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    u_coords = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
    v_coords = (float *)malloc_from_unmanaged_aligned(
        (SIMD_FLOAT_WIDTH + 0) * sizeof(float), 32);
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
    
    log_assert(zpolygons_to_render_size < ZPOLYGONS_TO_RENDER_ARRAYSIZE);
    
    // valid for this entire frame
    float camera_x_pos = camera.x;
    float camera_y_pos = camera.y;
    float camera_z_pos = camera.z;
    float cos_cam_x    = cosf(-camera.x_angle);
    float sin_cam_x    = sinf(-camera.x_angle);
    float cos_cam_y    = cosf(-camera.y_angle);
    float sin_cam_y    = sinf(-camera.y_angle);
    float cos_cam_z    = cosf(-camera.z_angle);
    float sin_cam_z    = sinf(-camera.z_angle);
    float cos_of_zero  =  1.0f;
    float minus_one    = -1.0f;
    
    // all zeros to represent the origin
    // float zero = 0.0f;
    // SIMD_FLOAT simd_all_zeros = simd_set_float(zero);
    
    // valid for this entire frame
    SIMD_FLOAT simd_camera_x_pos       = simd_set_float(camera_x_pos);
    SIMD_FLOAT simd_camera_y_pos       = simd_set_float(camera_y_pos);
    SIMD_FLOAT simd_camera_z_pos       = simd_set_float(camera_z_pos);
    SIMD_FLOAT simd_cos_camera_x_angle = simd_set_float(cos_cam_x);
    SIMD_FLOAT simd_sin_camera_x_angle = simd_set_float(sin_cam_x);
    SIMD_FLOAT simd_cos_camera_y_angle = simd_set_float(cos_cam_y);
    SIMD_FLOAT simd_sin_camera_y_angle = simd_set_float(sin_cam_y);
    SIMD_FLOAT simd_cos_camera_z_angle = simd_set_float(cos_cam_z);
    SIMD_FLOAT simd_sin_camera_z_angle = simd_set_float(sin_cam_z);
    SIMD_FLOAT simd_cos_of_zero        = simd_set_float(cos_of_zero);
    SIMD_FLOAT simd_minus_ones         = simd_set_float(minus_one);
    
    // valid for this entire frame
    zVertex origin;
    origin.x = 0.0f;
    origin.y = 0.0f;
    origin.z = 0.0f;
    
    uint32_t current_zp_i        = 0;
    uint32_t current_zp_tri_i    = 0;
    uint32_t current_zp_tri_m    = 0;
    uint32_t current_tq_i        = 0;
    uint32_t current_tq_vertex_i = 0;
    
    uint32_t cur_simd_vertex_offset_i = 0;
    
    Vertex vertices_to_render[SIMD_FLOAT_WIDTH];
    
    log_assert(*next_workload_size == 0);
    
    // TODO: it may be instructive to rewrite our data layout to minimmize this fetching & copying
    // TODO: even if it has no significant performance impact and takes a bunch of my time,
    // TODO: I want to try something like that at least once just for learning
    while (
        current_zp_i < zpolygons_to_render_size ||
        current_tq_i < texquads_to_render_size)
    {
        while (
            cur_simd_vertex_offset_i < SIMD_FLOAT_WIDTH &&
            (current_zp_i < zpolygons_to_render_size ||
                current_tq_i < texquads_to_render_size))
        {
            if (current_zp_i < zpolygons_to_render_size) {
                if (
                    current_zp_tri_i < zpolygons_to_render[current_zp_i].triangles_size)
                {
                    if (current_zp_tri_m < 3) {
                        triangle_vertices_x[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].
                                triangles[current_zp_tri_i].vertices[current_zp_tri_m].x;
                        triangle_normals_x[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].
                                triangles[current_zp_tri_i].normals[current_zp_tri_m].x;
                        triangle_vertices_y[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].
                                triangles[current_zp_tri_i].vertices[current_zp_tri_m].y;
                        triangle_normals_y[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].
                                triangles[current_zp_tri_i].normals[current_zp_tri_m].y;
                        triangle_vertices_z[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].
                                triangles[current_zp_tri_i].vertices[current_zp_tri_m].z;
                        triangle_normals_z[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].
                                triangles[current_zp_tri_i].normals[current_zp_tri_m].z;
                        parent_x[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].x;
                        parent_y[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].y;
                        parent_z[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].z;
                        
                        x_angles[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].x_angle;
                        y_angles[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].y_angle;
                        z_angles[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].z_angle;
                        
                        scale_factors[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].scale_factor;
                        
                        camera_multipliers[cur_simd_vertex_offset_i] =
                            (1.0f * !zpolygons_to_render[current_zp_i].ignore_camera);
                        lighting_multipliers[cur_simd_vertex_offset_i] =
                            (1.0f * !zpolygons_to_render[current_zp_i].ignore_lighting);
                        
                        touchable_ids[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].touchable_id;
                        
                        texturearray_indexes[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].
                                triangles[current_zp_tri_i].
                                texturearray_i;
                        texture_indexes[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].
                                triangles[current_zp_tri_i].
                                texture_i;
                        
                        rgba_reds[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].
                                triangles[current_zp_tri_i].
                                color[0];
                        rgba_greens[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].
                                triangles[current_zp_tri_i].
                                color[1];
                        rgba_blues[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].
                                triangles[current_zp_tri_i].
                                color[2];
                        rgba_alphas[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].
                                triangles[current_zp_tri_i].
                                color[3];
                        
                        u_coords[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].
                                triangles[current_zp_tri_i].
                                vertices[current_zp_tri_m].
                                uv[0];
                        v_coords[cur_simd_vertex_offset_i] =
                            zpolygons_to_render[current_zp_i].
                                triangles[current_zp_tri_i].
                                vertices[current_zp_tri_m].
                                uv[1];
                        
                        cur_simd_vertex_offset_i += 1;
                        current_zp_tri_m += 1;
                    } else {
                        current_zp_tri_i += 1;
                        current_zp_tri_m = 0;
                    }
                } else {
                    current_zp_tri_i = 0;
                    current_zp_tri_m = 0;
                    current_zp_i     += 1;
                }
            } else if (
                current_tq_i < texquads_to_render_size)
            {
                float left_uv_coord   = 0.0f;
                float right_uv_coord  = 1.0f;
                float bottom_uv_coord = 1.0f;
                float top_uv_coord    = 0.0f;
                
                float offset_left =
                    texquads_to_render[current_tq_i].left_x +
                        texquads_to_render[current_tq_i].x_offset;
                float offset_right =
                    offset_left + texquads_to_render[current_tq_i].width;
                float offset_mid_x = (offset_left + offset_right) / 2;
                
                float offset_top =
                    texquads_to_render[current_tq_i].top_y +
                        texquads_to_render[current_tq_i].y_offset;
                float offset_bottom =
                    offset_top - texquads_to_render[current_tq_i].height;
                float offset_mid_y = (offset_top + offset_bottom) / 2;
                
                if (current_tq_vertex_i == 0) {
                    // *** top left triangle
                    // topleft vertex
                    triangle_vertices_x[cur_simd_vertex_offset_i] = offset_left - offset_mid_x;
                    triangle_vertices_y[cur_simd_vertex_offset_i] = offset_top - offset_mid_y;
                    u_coords[cur_simd_vertex_offset_i] = left_uv_coord;
                    v_coords[cur_simd_vertex_offset_i] = top_uv_coord;
                } else if (current_tq_vertex_i == 1) {
                    // topright vertex
                    triangle_vertices_x[cur_simd_vertex_offset_i] = offset_right - offset_mid_x;
                    triangle_vertices_y[cur_simd_vertex_offset_i] = offset_top - offset_mid_y;
                    u_coords[cur_simd_vertex_offset_i] = right_uv_coord;
                    v_coords[cur_simd_vertex_offset_i] = top_uv_coord;
                } else if (current_tq_vertex_i == 2) {
                    // bottomleft vertex
                    triangle_vertices_x[cur_simd_vertex_offset_i] = offset_left - offset_mid_x;
                    triangle_vertices_y[cur_simd_vertex_offset_i] = offset_bottom - offset_mid_y;
                    u_coords[cur_simd_vertex_offset_i] = left_uv_coord;
                    v_coords[cur_simd_vertex_offset_i] = bottom_uv_coord;
                } else if (current_tq_vertex_i == 3) {
                    // *** bottom right triangle
                    // topright vertex
                    triangle_vertices_x[cur_simd_vertex_offset_i] = offset_right - offset_mid_x;
                    triangle_vertices_y[cur_simd_vertex_offset_i] = offset_top - offset_mid_y;
                    u_coords[cur_simd_vertex_offset_i] = right_uv_coord;
                    v_coords[cur_simd_vertex_offset_i] = top_uv_coord;
                } else if (current_tq_vertex_i == 4) {
                    // bottomright vertex
                    triangle_vertices_x[cur_simd_vertex_offset_i] = offset_right - offset_mid_x;
                    triangle_vertices_y[cur_simd_vertex_offset_i] = offset_bottom - offset_mid_y;
                    u_coords[cur_simd_vertex_offset_i] = right_uv_coord;
                    v_coords[cur_simd_vertex_offset_i] = bottom_uv_coord;
                } else if (current_tq_vertex_i == 5) {
                    // bottomleft vertex
                    triangle_vertices_x[cur_simd_vertex_offset_i] = offset_left - offset_mid_x;
                    triangle_vertices_y[cur_simd_vertex_offset_i] = offset_bottom - offset_mid_y;
                    u_coords[cur_simd_vertex_offset_i] = left_uv_coord;
                    v_coords[cur_simd_vertex_offset_i] = bottom_uv_coord;
                }
                
                triangle_vertices_z[cur_simd_vertex_offset_i] = 0.0f;
                
                triangle_normals_x[cur_simd_vertex_offset_i] = 0.0f;
                triangle_normals_y[cur_simd_vertex_offset_i] = 0.0f;
                triangle_normals_z[cur_simd_vertex_offset_i] = 1.0f;
                
                scale_factors[cur_simd_vertex_offset_i] =
                    texquads_to_render[current_tq_i].scale_factor;
                
                rgba_reds[cur_simd_vertex_offset_i] =
                    texquads_to_render[current_tq_i].RGBA[0];
                rgba_greens[cur_simd_vertex_offset_i] =
                    texquads_to_render[current_tq_i].RGBA[1];
                rgba_blues[cur_simd_vertex_offset_i] =
                    texquads_to_render[current_tq_i].RGBA[2];
                rgba_alphas[cur_simd_vertex_offset_i] =
                    texquads_to_render[current_tq_i].RGBA[3];
                
                texturearray_indexes[cur_simd_vertex_offset_i] =
                    texquads_to_render[current_tq_i].texturearray_i;
                texture_indexes[cur_simd_vertex_offset_i] =
                    texquads_to_render[current_tq_i].texture_i;
                
                camera_multipliers[cur_simd_vertex_offset_i] =
                    !texquads_to_render[current_tq_i].ignore_camera;
                
                lighting_multipliers[cur_simd_vertex_offset_i] =
                    !texquads_to_render[current_tq_i].ignore_camera;
                
                x_angles[cur_simd_vertex_offset_i] = 0.0f;
                y_angles[cur_simd_vertex_offset_i] = 0.0f;
                z_angles[cur_simd_vertex_offset_i] =
                    texquads_to_render[current_tq_i].z_angle;
                
                parent_x[cur_simd_vertex_offset_i] = offset_mid_x;
                parent_y[cur_simd_vertex_offset_i] = offset_mid_y;
                parent_z[cur_simd_vertex_offset_i] =
                    texquads_to_render[current_tq_i].z;;
                
                touchable_ids[cur_simd_vertex_offset_i] =
                    texquads_to_render[current_tq_i].touchable_id;
                
                current_tq_vertex_i += 1;
                if (current_tq_vertex_i > 5) {
                    current_tq_i += 1;
                    current_tq_vertex_i = 0;
                }
                cur_simd_vertex_offset_i += 1;
            }
        }
        
        int32_t underflow = SIMD_FLOAT_WIDTH - cur_simd_vertex_offset_i;
        log_assert(underflow >= 0);
        
        // apply scaling modifier
        SIMD_FLOAT scaling_modifiers = simd_load_floats(scale_factors);
        
        SIMD_FLOAT simd_vertices_x = simd_load_floats(triangle_vertices_x);
        SIMD_FLOAT simd_normals_x = simd_load_floats(triangle_normals_x);
        simd_vertices_x = simd_mul_floats(simd_vertices_x, scaling_modifiers);
        // simd_normals_x = simd_mul_floats(simd_normals_x, scaling_modifiers);
        
        SIMD_FLOAT simd_vertices_y = simd_load_floats(triangle_vertices_y);
        SIMD_FLOAT simd_normals_y = simd_load_floats(triangle_normals_y);
        simd_vertices_y = simd_mul_floats(simd_vertices_y, scaling_modifiers);
        // simd_normals_y = simd_mul_floats(simd_normals_y, scaling_modifiers);

        SIMD_FLOAT simd_vertices_z = simd_load_floats(triangle_vertices_z);
        SIMD_FLOAT simd_normals_z = simd_load_floats(triangle_normals_z);
        simd_vertices_z = simd_mul_floats(simd_vertices_z, scaling_modifiers);
        // simd_normals_z = simd_mul_floats(simd_normals_z, scaling_modifiers);
        
        // store original position
        for (uint32_t j = 0; j < SIMD_FLOAT_WIDTH - underflow; j++) {
            vertices_to_render[j].orig_x = simd_vertices_x[j];
            vertices_to_render[j].orig_y = simd_vertices_y[j];
            vertices_to_render[j].orig_z = simd_vertices_z[j];
        }
        
        // rotate vertices
        float cosines[SIMD_FLOAT_WIDTH];
        float sines[SIMD_FLOAT_WIDTH];
        for (uint32_t j = 0; j < SIMD_FLOAT_WIDTH - underflow; j++) {
            cosines[j] = cosf(x_angles[j]);
            sines[j]   = sinf(x_angles[j]);
        }
        SIMD_FLOAT simd_cosines = simd_load_floats(cosines);
        SIMD_FLOAT simd_sines   = simd_load_floats(sines);
        
        x_rotate_zvertices_inplace(
            /* SIMD_FLOAT * vec_to_rotate_y      : */
                &simd_vertices_y,
            /* SIMD_FLOAT * vec_to_rotate_z      : */
                &simd_vertices_z,
            /* const SIMD_FLOAT cos_angles       : */
                simd_cosines,
            /* const SIMD_FLOAT sin_angles       : */
                simd_sines);
        x_rotate_zvertices_inplace(
            /* SIMD_FLOAT * vec_to_rotate_y      : */
                &simd_normals_y,
            /* SIMD_FLOAT * vec_to_rotate_z      : */
                &simd_normals_z,
            /* const SIMD_FLOAT cos_angles       : */
                simd_cosines,
            /* const SIMD_FLOAT sin_angles       : */
                simd_sines);
        
        for (uint32_t j = 0; j < SIMD_FLOAT_WIDTH - underflow; j++) {
            cosines[j] = cosf(y_angles[j]);
            sines[j]   = sinf(y_angles[j]);
        }
        simd_cosines = simd_load_floats(cosines);
        simd_sines   = simd_load_floats(sines);
        y_rotate_zvertices_inplace(
            /* SIMD_FLOAT * vec_to_rotate_x      : */
                &simd_vertices_x,
            /* SIMD_FLOAT * vec_to_rotate_z      : */
                &simd_vertices_z,
            /* const SIMD_FLOAT cos_angles       : */
                simd_cosines,
            /* const SIMD_FLOAT sin_angles       : */
                simd_sines);
        y_rotate_zvertices_inplace(
            /* SIMD_FLOAT * vec_to_rotate_x      : */
                &simd_normals_x,
            /* SIMD_FLOAT * vec_to_rotate_z      : */
                &simd_normals_z,
            /* const SIMD_FLOAT cos_angles       : */
                simd_cosines,
            /* const SIMD_FLOAT sin_angles       : */
                simd_sines);
        
        for (uint32_t j = 0; j < SIMD_FLOAT_WIDTH - underflow; j++) {
            cosines[j] = cosf(z_angles[j]);
            sines[j]   = sinf(z_angles[j]);
        }
        simd_cosines = simd_load_floats(cosines);
        simd_sines   = simd_load_floats(sines);
        z_rotate_zvertices_inplace(
            /* SIMD_FLOAT * vec_to_rotate_x      : */
                &simd_vertices_x,
            /* SIMD_FLOAT * vec_to_rotate_y      : */
                &simd_vertices_y,
            /* const SIMD_FLOAT cos_angles       : */
                simd_cosines,
            /* const SIMD_FLOAT sin_angles       : */
                simd_sines);
        z_rotate_zvertices_inplace(
            /* SIMD_FLOAT * vec_to_rotate_x      : */
                &simd_normals_x,
            /* SIMD_FLOAT * vec_to_rotate_y      : */
                &simd_normals_y,
            /* const SIMD_FLOAT cos_angles       : */
                simd_cosines,
            /* const SIMD_FLOAT sin_angles       : */
                simd_sines);
        
        SIMD_FLOAT simd_polygons_x = simd_load_floats(parent_x);
        SIMD_FLOAT simd_polygons_y = simd_load_floats(parent_y);
        SIMD_FLOAT simd_polygons_z = simd_load_floats(parent_z);
        simd_vertices_x = simd_add_floats(simd_vertices_x, simd_polygons_x);
        simd_vertices_y = simd_add_floats(simd_vertices_y, simd_polygons_y);
        simd_vertices_z = simd_add_floats(simd_vertices_z, simd_polygons_z);
//        simd_normals_x = simd_add_floats(simd_normals_x, simd_polygons_x);
//        simd_normals_y = simd_add_floats(simd_normals_y, simd_polygons_y);
//        simd_normals_z = simd_add_floats(simd_normals_z, simd_polygons_z);
        
        // these are always 1 or 0, 0 for 'ignore camera' vertices
        SIMD_FLOAT simd_camera_multipliers =
            simd_load_floats(camera_multipliers);
        SIMD_FLOAT simd_reverse_camera_multipliers =
            simd_add_floats(
                simd_camera_multipliers,
                simd_minus_ones);
        simd_reverse_camera_multipliers =
            simd_mul_floats(
                simd_reverse_camera_multipliers,
                simd_minus_ones);
        
        // translate the world so that the camera becomes 0,0,0
        SIMD_FLOAT simd_camera_mod = simd_camera_x_pos * simd_camera_multipliers;
        simd_vertices_x = simd_sub_floats(simd_vertices_x, simd_camera_mod);
//        simd_normals_x = simd_sub_floats(simd_normals_x, simd_camera_mod);
        simd_camera_mod = simd_camera_y_pos * simd_camera_multipliers;
        simd_vertices_y = simd_sub_floats(simd_vertices_y, simd_camera_mod);
//        simd_normals_y = simd_sub_floats(simd_normals_y, simd_camera_mod);
        simd_camera_mod = simd_camera_z_pos * simd_camera_multipliers;
        simd_vertices_z = simd_sub_floats(simd_vertices_z, simd_camera_mod);
//        simd_normals_z = simd_sub_floats(simd_normals_z, simd_camera_mod);
        
        // rotate the world so that the camera's angles all become 0
        SIMD_FLOAT simd_cam_active_cos_angle = simd_mul_floats(
            simd_cos_camera_x_angle, simd_camera_multipliers);
        SIMD_FLOAT simd_cam_inactive_cos_angle = simd_mul_floats(
            simd_cos_of_zero, simd_reverse_camera_multipliers);
        SIMD_FLOAT simd_final_cos_angle = simd_add_floats(
            simd_cam_active_cos_angle, simd_cam_inactive_cos_angle);
        // reminder: the sin of 0 is 0, so this works out naturally
        SIMD_FLOAT simd_final_sin_angle = simd_mul_floats(
            simd_sin_camera_x_angle, simd_camera_multipliers);
        x_rotate_zvertices_inplace(
            /* SIMD_FLOAT * vec_to_rotate_y      : */
                &simd_vertices_y,
            /* SIMD_FLOAT * vec_to_rotate_z      : */
                &simd_vertices_z,
            /* const SIMD_FLOAT cos_angles       : */
                simd_final_cos_angle,
            /* const SIMD_FLOAT sin_angles       : */
                simd_final_sin_angle);
//        x_rotate_zvertices_inplace(
//            /* SIMD_FLOAT * vec_to_rotate_y      : */
//                &simd_normals_y,
//            /* SIMD_FLOAT * vec_to_rotate_z      : */
//                &simd_normals_z,
//            /* const SIMD_FLOAT cos_angles       : */
//                simd_final_cos_angle,
//            /* const SIMD_FLOAT sin_angles       : */
//                simd_final_sin_angle);
        
        simd_cam_active_cos_angle = simd_mul_floats(
            simd_cos_camera_y_angle,
            simd_camera_multipliers);
        simd_cam_inactive_cos_angle = simd_mul_floats(
            simd_cos_of_zero,
            simd_reverse_camera_multipliers);
        simd_final_cos_angle = simd_add_floats(
            simd_cam_active_cos_angle,
            simd_cam_inactive_cos_angle);
        // reminder: the sin of 0 is 0, so this works out naturally
        simd_final_sin_angle = simd_mul_floats(
            simd_sin_camera_y_angle,
            simd_camera_multipliers);
        y_rotate_zvertices_inplace(
            /* SIMD_FLOAT * vec_to_rotate_x      : */
                &simd_vertices_x,
            /* SIMD_FLOAT * vec_to_rotate_z      : */
                &simd_vertices_z,
            /* const SIMD_FLOAT cos_angles       : */
                simd_final_cos_angle,
            /* const SIMD_FLOAT sin_angles       : */
                simd_final_sin_angle);
//        y_rotate_zvertices_inplace(
//            /* SIMD_FLOAT * vec_to_rotate_x      : */
//                &simd_normals_x,
//            /* SIMD_FLOAT * vec_to_rotate_z      : */
//                &simd_normals_z,
//            /* const SIMD_FLOAT cos_angles       : */
//                simd_final_cos_angle,
//            /* const SIMD_FLOAT sin_angles       : */
//                simd_final_sin_angle);
        
        simd_cam_active_cos_angle = simd_mul_floats(
            simd_cos_camera_z_angle, simd_camera_multipliers);
        simd_cam_inactive_cos_angle = simd_mul_floats(
            simd_cos_of_zero, simd_reverse_camera_multipliers);
        simd_final_cos_angle = simd_add_floats(
            simd_cam_active_cos_angle, simd_cam_inactive_cos_angle);
        // reminder: the sin of 0 is 0, so this works out naturally
        simd_final_sin_angle = simd_mul_floats(
            simd_sin_camera_z_angle, simd_camera_multipliers);
        z_rotate_zvertices_inplace(
            /* SIMD_FLOAT * vec_to_rotate_x      : */
                &simd_vertices_x,
            /* SIMD_FLOAT * vec_to_rotate_y      : */
                &simd_vertices_y,
            /* const SIMD_FLOAT cos_angles       : */
                simd_final_cos_angle,
            /* const SIMD_FLOAT sin_angles       : */
                simd_final_sin_angle);
//        z_rotate_zvertices_inplace(
//            /* SIMD_FLOAT * vec_to_rotate_x      : */
//                &simd_normals_x,
//            /* SIMD_FLOAT * vec_to_rotate_z      : */
//                &simd_normals_y,
//            /* const SIMD_FLOAT cos_angles       : */
//                simd_final_cos_angle,
//            /* const SIMD_FLOAT sin_angles       : */
//                simd_final_sin_angle);
        
        // simd_store_floats(triangle_normals_x, simd_normals_x);
        // simd_store_floats(triangle_normals_y, simd_normals_y);
        // simd_store_floats(triangle_normals_z, simd_normals_z);
        for (uint32_t j = 0; j < SIMD_FLOAT_WIDTH - underflow; j++) {
            vertices_to_render[j].normal_x       = simd_normals_x[j];
            vertices_to_render[j].normal_y       = simd_normals_y[j];
            vertices_to_render[j].normal_z       = simd_normals_z[j];
            vertices_to_render[j].w              = simd_vertices_z[j];
            vertices_to_render[j].texturearray_i = texturearray_indexes[j];
            vertices_to_render[j].texture_i      = texture_indexes[j];
            vertices_to_render[j].RGBA[0]        = rgba_reds[j];
            vertices_to_render[j].RGBA[1]        = rgba_greens[j];
            vertices_to_render[j].RGBA[2]        = rgba_blues[j];
            vertices_to_render[j].RGBA[3]        = rgba_alphas[j];
            vertices_to_render[j].uv[0]          = u_coords[j];
            vertices_to_render[j].uv[1]          = v_coords[j];
            vertices_to_render[j].touchable_id   = touchable_ids[j];
        }
        
        project_simd_vertices_to_2d(
            /* SIMD_FLOAT * simd_vertices_x: */
                &simd_vertices_x,
            /* SIMD_FLOAT * simd_vertices_y: */
                &simd_vertices_y,
            /* SIMD_FLOAT * simd_vertices_z: */
                &simd_vertices_z);
        
        for (uint32_t j = 0; j < SIMD_FLOAT_WIDTH - underflow; j++) {
            vertices_to_render[j].x = simd_vertices_x[j];
            vertices_to_render[j].y = simd_vertices_y[j];
            vertices_to_render[j].z = simd_vertices_z[j];
        }
        
        draw_vertices(
            /* vertices_recipient: */
                next_gpu_workload,
            /* vertex_count_recipient: */
                next_workload_size,
            /* input: */
                vertices_to_render,
            /* input_size: */
                SIMD_FLOAT_WIDTH - underflow);
        
        cur_simd_vertex_offset_i = 0;
    }
}
