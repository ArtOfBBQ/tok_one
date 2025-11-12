#include "T1_renderer.h"

static uint32_t renderer_initialized = false;

void renderer_init(void) {
    renderer_initialized = true;
    
    T1_std_memset(&camera, 0, sizeof(T1GPUCamera));
}

#if T1_RAW_SHADER_ACTIVE == T1_ACTIVE
static void add_line_vertex(
    GPUDataForSingleFrame * frame_data,
    const float xyz[3])
{
    log_assert(frame_data->line_vertices != NULL);
    
    if (frame_data->line_vertices_size >= MAX_LINE_VERTICES) {
        return;
    }
    
    common_memcpy(
        &frame_data->line_vertices[frame_data->line_vertices_size].xyz,
        xyz,
        sizeof(float) * 3);
    
    frame_data->line_vertices[frame_data->line_vertices_size].color =
        0.0f;
    
    frame_data->line_vertices_size += 1;
}

static void add_point_vertex(
    GPUDataForSingleFrame * frame_data,
    const float xyz[3],
    float color)
{
    log_assert(frame_data->point_vertices != NULL);
    
    if (frame_data->point_vertices_size >= MAX_POINT_VERTICES) {
        return;
    }
    
    common_memcpy(
        &frame_data->point_vertices[frame_data->point_vertices_size].xyz,
        xyz,
        sizeof(float) * 3);
    
    frame_data->point_vertices[frame_data->point_vertices_size].color = color;
    
    frame_data->point_vertices_size += 1;
}

#if 0
inline static void draw_bounding_sphere(
    GPUDataForSingleFrame * frame_data,
    const float center_xyz[3],
    const float sphere_radius)
{
    float cur_point[3];
    for (float x_angle = 0.2f; x_angle < 6.28f; x_angle += 0.75f)
    {
        for (float y_angle = 0.0f; y_angle < 6.28f; y_angle += 0.75f)
        {
            cur_point[0] = 0.0f;
            cur_point[1] = 0.0f;
            cur_point[2] = sphere_radius;
            
            x_rotate_f3(cur_point, x_angle);
            y_rotate_f3(cur_point, y_angle);
            
            cur_point[0] += center_xyz[0];
            cur_point[1] += center_xyz[1];
            cur_point[2] += center_xyz[2];
            
            add_point_vertex(frame_data, cur_point, /* color: */ 0.0f);
        }
    }
}
#endif

#elif T1_RAW_SHADER_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_RAW_SHADER_ACTIVE undefined"
#endif

inline static void add_alphablending_zpolygons_to_workload(
    T1GPUFrame * frame_data)
{
    frame_data->first_alphablend_i = frame_data->verts_size;
    
    // Copy all vertices that do use alpha blending
    for (
        int32_t cpu_zp_i = 0;
        cpu_zp_i < (int32_t)T1_zsprites_to_render->size;
        cpu_zp_i++)
    {
        if (
            T1_zsprites_to_render->cpu_data[cpu_zp_i].deleted ||
            !T1_zsprites_to_render->cpu_data[cpu_zp_i].visible ||
            !T1_zsprites_to_render->cpu_data[cpu_zp_i].committed ||
            !T1_zsprites_to_render->cpu_data[cpu_zp_i].alpha_blending_enabled)
        {
            continue;
        }
        
        int32_t mesh_id = T1_zsprites_to_render->cpu_data[cpu_zp_i].mesh_id;
        log_assert(mesh_id >= 0);
        log_assert(mesh_id < (int32_t)all_mesh_summaries_size);
        
        int32_t vert_tail_i =
            all_mesh_summaries[mesh_id].vertices_head_i +
                all_mesh_summaries[mesh_id].vertices_size;
        assert(vert_tail_i < MAX_VERTICES_PER_BUFFER);
        
        for (
            int32_t vert_i = all_mesh_summaries[mesh_id].vertices_head_i;
            vert_i < vert_tail_i;
            vert_i += 1)
        {
            frame_data->verts[frame_data->verts_size].locked_vertex_i =
                vert_i;
            frame_data->verts[frame_data->verts_size].polygon_i =
                cpu_zp_i;
            frame_data->verts_size += 1;
            log_assert(frame_data->verts_size < MAX_VERTICES_PER_BUFFER);
        }
    }
    
    #if 0
    if (
        frame_data->vertices_size > frame_data->first_alphablend_i)
    {
        log_assert(
            (frame_data->vertices_size - frame_data->first_alphablend_i) % 3 == 0);
        log_assert(frame_data->first_alphablend_i % 3 == 0);
        
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        uint32_t initial_first_alphablend_i = frame_data->first_alphablend_i;
        uint32_t initial_vertices_size = frame_data->vertices_size;
        
        for (
            int32_t i = (int)frame_data->first_alphablend_i;
            i < (int)frame_data->vertices_size;
            i += 3)
        {
            log_assert(
                frame_data->vertices[i+0].polygon_i ==
                frame_data->vertices[i+1].polygon_i);
            log_assert(
                frame_data->vertices[i+0].polygon_i ==
                frame_data->vertices[i+2].polygon_i);
            log_assert(
                frame_data->vertices[i+0].locked_vertex_i !=
                frame_data->vertices[i+1].locked_vertex_i);
            log_assert(
                frame_data->vertices[i+0].locked_vertex_i !=
                frame_data->vertices[i+2].locked_vertex_i);
            log_assert(
                frame_data->vertices[i+1].locked_vertex_i !=
                frame_data->vertices[i+2].locked_vertex_i);
        }
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        // Pass
        #else
        #error "T1_LOGGER_ASSERTS_ACTIVE undefined"
        #endif
        
        qsort(
            /* base: */
                frame_data->vertices + frame_data->first_alphablend_i,
            /* size_t nel: */
                (frame_data->vertices_size -
                    frame_data->first_alphablend_i) / 3,
            /* size_t width: */
                sizeof(GPUVertexIndices) * 3,
            /* int (* _Nonnull compar)(const void *, const void *): */
                compare_triangles_furthest_camera_dist);
        
        #if T1_LOGGER_ASSERTS_ACTIVE
        log_assert(
            frame_data->first_alphablend_i == initial_first_alphablend_i);
        log_assert(frame_data->vertices_size == initial_vertices_size);
        for (
            int32_t i = (int)frame_data->first_alphablend_i;
            i < (int)frame_data->vertices_size;
            i += 3)
        {
            log_assert(
                frame_data->vertices[i+0].polygon_i ==
                frame_data->vertices[i+1].polygon_i);
            log_assert(
                frame_data->vertices[i+0].polygon_i ==
                frame_data->vertices[i+2].polygon_i);
            log_assert(
                frame_data->vertices[i+0].locked_vertex_i !=
                frame_data->vertices[i+1].locked_vertex_i);
            log_assert(
                frame_data->vertices[i+0].locked_vertex_i !=
                frame_data->vertices[i+2].locked_vertex_i);
            log_assert(
                frame_data->vertices[i+1].locked_vertex_i !=
                frame_data->vertices[i+2].locked_vertex_i);
        }
        #endif
    }
    #endif
}

inline static void add_opaque_zpolygons_to_workload(
    T1GPUFrame * frame_data)
{
    log_assert(frame_data->verts_size == 0);
    
    int32_t cur_vals[4];
    int32_t incr_vals[4];
    incr_vals[0] = 2;
    incr_vals[1] = 0;
    incr_vals[2] = 2;
    incr_vals[3] = 0;
    SIMD_VEC4I incr = simd_load_vec4i(incr_vals);
    
    for (
        int32_t cpu_zp_i = 0;
        cpu_zp_i < (int32_t)T1_zsprites_to_render->size;
        cpu_zp_i++)
    {
        if (
            T1_zsprites_to_render->cpu_data[cpu_zp_i].deleted ||
            !T1_zsprites_to_render->cpu_data[cpu_zp_i].visible ||
            !T1_zsprites_to_render->cpu_data[cpu_zp_i].committed ||
            T1_zsprites_to_render->cpu_data[cpu_zp_i].alpha_blending_enabled)
        {
            continue;
        }
        
        int32_t mesh_id = T1_zsprites_to_render->
            cpu_data[cpu_zp_i].mesh_id;
        log_assert(mesh_id >= 0);
        log_assert(mesh_id < (int32_t)all_mesh_summaries_size);
        
        int32_t vert_tail_i =
            all_mesh_summaries[mesh_id].
                vertices_head_i +
                    all_mesh_summaries[mesh_id].
                        vertices_size;
        log_assert(
            vert_tail_i < MAX_VERTICES_PER_BUFFER);
        
        /*
        We are free to overflow the vertices buffer, since its end is not
        in use yet anyway.
        */
        int32_t vert_i = all_mesh_summaries[mesh_id].vertices_head_i;
        cur_vals[0] = vert_i-2;
        cur_vals[1] = cpu_zp_i;
        cur_vals[2] = vert_i-1;
        cur_vals[3] = cpu_zp_i;
        SIMD_VEC4I cur  = simd_load_vec4i(cur_vals);
        
        int32_t verts_to_copy = vert_tail_i - vert_i;
        
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        uint32_t previous_verts_size = frame_data->verts_size;
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        // Pass
        #else
        #error "T1_LOGGER_ASSERTS_ACTIVE undefined"
        #endif
        
        for (int32_t i = 0; i < verts_to_copy; i += 2) {
            cur = simd_add_vec4i(cur, incr);
            simd_store_vec4i(
                (frame_data->verts + frame_data->verts_size),
                cur);
            frame_data->verts_size += 2;
            
            #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
            log_assert(frame_data->verts_size < MAX_VERTICES_PER_BUFFER);
            log_assert(frame_data->verts[frame_data->verts_size-2].
                locked_vertex_i == (vert_i + i));
            log_assert(frame_data->verts[frame_data->verts_size-1].
                locked_vertex_i == (vert_i + i + 1));
            #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
            // Pass
            #else
            #error "T1_LOGGER_ASSERTS_ACTIVE undefined"
            #endif
        }
        
        if (verts_to_copy % 2 == 1) {
            frame_data->verts_size -= 1;
        }
        
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        log_assert(frame_data->verts_size ==
            (previous_verts_size + (uint32_t)verts_to_copy));
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        // Pass
        #else
        #error "T1_LOGGER_ASSERTS_ACTIVE undefined"
        #endif
    }
}

static void construct_projection_matrix(void) {
    
    T1GPUProjectConsts * p = &T1_engine_globals->project_consts;
    
    T1_linal_float4x4 proj;
    const float y_scale = p->field_of_view_modifier;
    const float x_scale = p->x_multiplier;
    
    const float tan_half_fov_y = 1.0f / y_scale;
    const float fov_y_rad = 2.0f * atanf(tan_half_fov_y);
    const float aspect = y_scale / x_scale;
    
    const float zn = p->znear;
    const float zf = p->zfar;
    
    const float f = 1.0f / tanf(fov_y_rad * 0.5f);
    
    #if 1
    // perspective projection
    T1_linal_float4x4_construct(
        &proj,
        f / aspect, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, zf / (zf - zn), -zf * zn / (zf - zn),
        0.0f, 0.0f, 1.0f, 0.0f
    );
    #else
    // orthographic projection
    T1_linal_float4x4_construct(
        &proj,
        1.0f,  0.0f,  0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.01f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    #endif
    
    T1_std_memcpy(
        camera.projection_4x4 + 0,
        proj.rows[0].data,
        sizeof(float) * 4);
    T1_std_memcpy(
        camera.projection_4x4 + 4,
        proj.rows[1].data,
        sizeof(float) * 4);
    T1_std_memcpy(
        camera.projection_4x4 + 8,
        proj.rows[2].data,
        sizeof(float) * 4);
    T1_std_memcpy(
        camera.projection_4x4 + 12,
        proj.rows[3].data,
        sizeof(float) * 4);
}

static void construct_view_matrix(void) {
    camera.xyz_cosangle[0] = cosf(camera.xyz_angle[0]);
    camera.xyz_cosangle[1] = cosf(camera.xyz_angle[1]);
    camera.xyz_cosangle[2] = cosf(camera.xyz_angle[2]);
    camera.xyz_sinangle[0] = sinf(camera.xyz_angle[0]);
    camera.xyz_sinangle[1] = sinf(camera.xyz_angle[1]);
    camera.xyz_sinangle[2] = sinf(camera.xyz_angle[2]);
    
    T1_linal_float4x4 result;
    T1_linal_float4x4 next;
    
    T1_linal_float4x4_construct_identity(&result);
    
    T1_linal_float4x4_construct_xyz_rotation(
        &next,
        -camera.xyz_angle[0],
        -camera.xyz_angle[1],
        -camera.xyz_angle[2]);
    
    T1_linal_float4x4_mul_float4x4_inplace(
        &result,
        &next);
    
    T1_linal_float4x4_construct(
        &next,
        1.0f, 0.0f, 0.0f, -camera.xyz[0],
        0.0f, 1.0f, 0.0f, -camera.xyz[1],
        0.0f, 0.0f, 1.0f, -camera.xyz[2],
        0.0f, 0.0f, 0.0f, 1.0f);
    
    T1_linal_float4x4_mul_float4x4_inplace(
        &result, &next);
    
    T1_std_memcpy(
        camera.view_4x4 + 0,
        result.rows[0].data,
        sizeof(float) * 4);
    T1_std_memcpy(
        camera.view_4x4 + 4,
        result.rows[1].data,
        sizeof(float) * 4);
    T1_std_memcpy(
        camera.view_4x4 + 8,
        result.rows[2].data,
        sizeof(float) * 4);
    T1_std_memcpy(
        camera.view_4x4 + 12,
        result.rows[3].data,
        sizeof(float) * 4);
}

static void construct_light_matrices(
    T1GPUFrame * frame_data)
{
    T1_linal_float4x4 mat_view;
    T1_linal_float4x4_construct_from_ptr(
        &mat_view,
        camera.view_4x4);
    
    T1_linal_float4x4 inv_camview_4x4;
    T1_std_memcpy(
        &inv_camview_4x4,
        &mat_view,
        sizeof(T1_linal_float4x4));
    
    T1_linal_float4x4_inverse_inplace(
        &inv_camview_4x4);
    
    T1_linal_float4x4 a_4x4;
    T1_linal_float4x4 b_4x4;
    
    for (
        uint32_t light_i = 0;
        light_i < zlights_to_apply_size;
        light_i++)
    {
        T1_linal_float4 light_world;
        light_world.data[0] =
            zlights_to_apply[light_i].xyz[0];
        light_world.data[1] =
            zlights_to_apply[light_i].xyz[1];
        light_world.data[2] =
            zlights_to_apply[light_i].xyz[2];
        light_world.data[3] = 1.0f;
        
        T1_linal_float4 view_pos =
            T1_linal_float4x4_mul_float4(
                &mat_view,
                light_world);
        
        frame_data->lights[light_i].viewspace_xyz[0] =
            view_pos.data[0];
        frame_data->lights[light_i].viewspace_xyz[1] =
            view_pos.data[1];
        frame_data->lights[light_i].viewspace_xyz[2] =
            view_pos.data[2];
        
        // Next, we want to transform from camera view to light view
        T1_linal_float4x4_construct_xyz_rotation(
            &a_4x4,
            -zlights_to_apply[light_i].xyz_angle[0],
            -zlights_to_apply[light_i].xyz_angle[1],
            -zlights_to_apply[light_i].xyz_angle[2]);
        
        T1_linal_float4x4_construct(
            &b_4x4,
            1.0f, 0.0f, 0.0f, -zlights_to_apply[light_i].xyz[0],
            0.0f, 1.0f, 0.0f, -zlights_to_apply[light_i].xyz[1],
            0.0f, 0.0f, 1.0f, -zlights_to_apply[light_i].xyz[2],
            0.0f, 0.0f, 0.0f, 1.0f);
        
        T1_linal_float4x4_mul_float4x4_inplace(
            &a_4x4, &b_4x4);
        
        T1_linal_float4x4_mul_float4x4_inplace(
            &a_4x4, &inv_camview_4x4);
        
        T1_std_memcpy(
            frame_data->lights[light_i].
                camview_to_lightview_4x4 + 0,
            a_4x4.rows[0].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            frame_data->lights[light_i].
                camview_to_lightview_4x4 + 4,
            a_4x4.rows[1].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            frame_data->lights[light_i].
                camview_to_lightview_4x4 + 8,
            a_4x4.rows[2].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            frame_data->lights[light_i].
                camview_to_lightview_4x4 + 12,
            a_4x4.rows[3].data,
            sizeof(float) * 4);
    }
}

static void construct_model_and_normal_matrices(void)
{
    T1_linal_float4x4 result;
    T1_linal_float4x4 next;
    
    T1_linal_float3x3 model3x3;
    T1_linal_float3x3 view3x3;
    
    for (uint32_t i = 0; i < T1_zsprites_to_render->size; i++) {
        
        T1CPUzSpriteSimdStats * s =
            &T1_zsprites_to_render->cpu_data[i].
                simd_stats;
        
        T1_linal_float4x4_construct_identity(&result);
        
        // Translation
        T1_linal_float4x4_construct(
            &next,
            1.0f, 0.0f, 0.0f, s->xyz[0],
            0.0f, 1.0f, 0.0f, s->xyz[1],
            0.0f, 0.0f, 1.0f, s->xyz[2],
            0.0f, 0.0f, 0.0f, 1.0f);
        
        T1_linal_float4x4_mul_float4x4_inplace(
            &result, &next);
        
        T1_linal_float4x4_construct_xyz_rotation(
            &next,
            s->angle_xyz[0],
            s->angle_xyz[1],
            s->angle_xyz[2]);
        
        T1_linal_float4x4_mul_float4x4_inplace(
            &result,
            &next);
        
        T1_linal_float4x4_construct(
            &next,
            1.0f, 0.0f, 0.0f, s->offset_xyz[0],
            0.0f, 1.0f, 0.0f, s->offset_xyz[1],
            0.0f, 0.0f, 1.0f, s->offset_xyz[2],
            0.0f, 0.0f, 0.0f, 1.0f);
        
        T1_linal_float4x4_mul_float4x4_inplace(
            &result, &next);
        
        T1_linal_float4x4_construct(
            &next,
            s->mul_xyz[0], 0.0f, 0.0f, 0.0f,
            0.0f, s->mul_xyz[1], 0.0f, 0.0f,
            0.0f, 0.0f, s->mul_xyz[2], 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
        T1_linal_float4x4_mul_float4x4_inplace(
            &result, &next);
        
        T1_std_memcpy(
            T1_zsprites_to_render->gpu_data[i].
                model_4x4 + 0,
            result.rows[0].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            T1_zsprites_to_render->gpu_data[i].
                model_4x4 + 4,
            result.rows[1].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            T1_zsprites_to_render->gpu_data[i].
                model_4x4 + 8,
            result.rows[2].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            T1_zsprites_to_render->gpu_data[i].
                model_4x4 + 12,
            result.rows[3].data,
            sizeof(float) * 4);
        
        // Store topleft 3x3 of "model to world"
        // matrix in result
        T1_linal_float4x4_extract_float3x3(
            /* const T1_linal_float4x4 * in: */
                &result,
            /* const int omit_row_i: */
                3,
            /* const int omit_col_i: */
                3,
            /* T1_linal_float3x3 * out: */
                &model3x3);
        
        // store topleft 3x3 of camera's "to view"
        // matrix in view3x3
        T1_linal_float3x3_construct(
            &view3x3,
            camera.view_4x4[ 0],
            camera.view_4x4[ 1],
            camera.view_4x4[ 2],
            camera.view_4x4[ 4],
            camera.view_4x4[ 5],
            camera.view_4x4[ 6],
            camera.view_4x4[ 8],
            camera.view_4x4[ 9],
            camera.view_4x4[10]);
        
        // view3x3 = view3x3 %*% model3x3
        // so now view3x3 is a "model to camera view"
        T1_linal_float3x3_mul_float3x3_inplace(
            &view3x3, &model3x3);
        
        // inverse transpose the "model to cam view"
        T1_linal_float3x3_inverse_transpose_inplace(&view3x3);
        
        // send to gpu
        T1_zsprites_to_render->gpu_data[i].
            normal_3x3[0] = view3x3.rows[0].data[0];
        T1_zsprites_to_render->gpu_data[i].
            normal_3x3[1] = view3x3.rows[0].data[1];
        T1_zsprites_to_render->gpu_data[i].
            normal_3x3[2] = view3x3.rows[0].data[2];
        T1_zsprites_to_render->gpu_data[i].
            normal_3x3[3] = view3x3.rows[1].data[0];
        T1_zsprites_to_render->gpu_data[i].
            normal_3x3[4] = view3x3.rows[1].data[1];
        T1_zsprites_to_render->gpu_data[i].
            normal_3x3[5] = view3x3.rows[1].data[2];
        T1_zsprites_to_render->gpu_data[i].
            normal_3x3[6] = view3x3.rows[2].data[0];
        T1_zsprites_to_render->gpu_data[i].
            normal_3x3[7] = view3x3.rows[2].data[1];
        T1_zsprites_to_render->gpu_data[i].
            normal_3x3[8] = view3x3.rows[2].data[2];
        
        T1_linal_float4x4_construct_from_ptr(
            &next,
            camera.view_4x4);
        
        T1_linal_float4x4_mul_float4x4_inplace(
            &next, &result);
        
        T1_std_memcpy(
            T1_zsprites_to_render->gpu_data[i].
                model_view_4x4 + 0,
            next.rows[0].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            T1_zsprites_to_render->gpu_data[i].
                model_view_4x4 + 4,
            next.rows[1].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            T1_zsprites_to_render->gpu_data[i].
                model_view_4x4 + 8,
            next.rows[2].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            T1_zsprites_to_render->gpu_data[i].
                model_view_4x4 + 12,
            next.rows[3].data,
            sizeof(float) * 4);
        
        float ic = T1_zsprites_to_render->
            cpu_data[i].simd_stats.ignore_camera;
        
        ic = T1_std_maxf(ic, 0.0f);
        ic = T1_std_minf(ic, 1.0f);
        
        for (uint32_t j = 0; j < 16; j++) {
            T1_zsprites_to_render->gpu_data[i].
                model_view_4x4[j] =
                    (T1_zsprites_to_render->gpu_data[i].
                        model_view_4x4[j] *
                            (1.0f - ic)) +
                    (T1_zsprites_to_render->gpu_data[i].
                        model_4x4[j] * ic);
        }
    }
}

void renderer_hardware_render(
    T1GPUFrame * frame_data,
    uint64_t elapsed_us)
{
    (void)elapsed_us;
    
    if (renderer_initialized != true) {
        log_append("renderer not initialized, aborting...\n");
        return;
    }
    
    if (
        frame_data == NULL ||
        frame_data->verts == NULL)
    {
        log_append("ERROR: platform layer didnt pass recipients\n");
        return;
    }
    
    log_assert(T1_zsprites_to_render->size < MAX_ZSPRITES_PER_BUFFER);
    
    construct_view_matrix();
    
    construct_projection_matrix();
    
    construct_model_and_normal_matrices();
    
    construct_light_matrices(frame_data);
    
    T1_std_memcpy(
        /* void * dest: */
            frame_data->zsprite_list->polygons,
        /* const void * src: */
            T1_zsprites_to_render->gpu_data,
        /* size_t n: */
            sizeof(T1GPUzSprite) * T1_zsprites_to_render->size);
    frame_data->zsprite_list->size = T1_zsprites_to_render->size;
    
    log_assert(
        frame_data->zsprite_list->size <= T1_zsprites_to_render->size);
    log_assert(
        T1_zsprites_to_render->size < MAX_ZSPRITES_PER_BUFFER);
    log_assert(
        frame_data->zsprite_list->size < MAX_ZSPRITES_PER_BUFFER);
    
    frame_data->zsprite_list->size = T1_zsprites_to_render->size;
    
    *frame_data->postproc_consts =
        T1_engine_globals->postproc_consts;
    
    add_opaque_zpolygons_to_workload(frame_data);
    
    if (T1_app_running) {
        #if T1_PARTICLES_ACTIVE == T1_ACTIVE
        T1_particle_add_all_to_frame_data(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* uint64_t elapsed_us: */
                elapsed_us,
            /* const uint32_t alpha_blending: */
                false);
        
        #if 0
        T1_particle_lineparticle_add_all_to_frame_data(
            frame_data,
            elapsed_us,
            false);
        #endif
        #elif T1_PARTICLES_ACTIVE == T1_INACTIVE
        // Pass
        #else
        #error "T1_PARTICLES_ACTIVE undefined"
        #endif
    }
    
    add_alphablending_zpolygons_to_workload(frame_data);
    
    #if T1_PARTICLES_ACTIVE == T1_ACTIVE
    T1_particle_add_all_to_frame_data(
        /* GPUDataForSingleFrame * frame_data: */
            frame_data,
        /* uint64_t elapsed_us: */
            elapsed_us,
        /* const uint32_t alpha_blending: */
            true);
    
    #if 0
    T1_particle_lineparticle_add_all_to_frame_data(
            frame_data,
            elapsed_us,
            true);
    #endif
    
    #elif T1_PARTICLES_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error
    #endif
    
    #if T1_RAW_SHADER_ACTIVE == T1_ACTIVE
    add_points_and_lines_to_workload(frame_data);
    
    if (application_running && engine_globals->draw_axes) {
        // TODO: draw axes
        float axis_vertices[6];
        common_memset_float(axis_vertices, 0.0f, sizeof(float) * 6);
        
        #define DISTANT_FLOAT 3.5f
        
        add_point_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float xyz[3]: */
                axis_vertices,
            /* const float color: */
                3.0f);
        add_line_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float xyz[3]: */
                axis_vertices);
        axis_vertices[3] = DISTANT_FLOAT;
        add_line_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float xyz[3]: */
                axis_vertices + 3);
        
        add_line_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float xyz[3]: */
                axis_vertices);
        axis_vertices[3] =  0.0f;
        axis_vertices[4] =  DISTANT_FLOAT;
        add_line_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float xyz[3]: */
                axis_vertices + 3);
        
        add_line_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float xyz[3]: */
                axis_vertices);
        axis_vertices[4] =  0.0f;
        axis_vertices[5] =  DISTANT_FLOAT;
        add_line_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float xyz[3]: */
                axis_vertices + 3);
    }
    
    if (
        application_running &&
        engine_globals->draw_imputed_normals)
    {
        assert(0);
    }
    
    if (application_running && engine_globals->draw_mouseptr) {
        float xyz[3];
        float z = 0.05f;
        xyz[0] = engineglobals_screenspace_x_to_x(
            user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].screen_x,
            z) + camera.xyz[0];
        xyz[1] = engineglobals_screenspace_y_to_y(
            user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].screen_y,
            z) + camera.xyz[1];
        xyz[2] = z + camera.xyz[2];
        add_point_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float * xyz: */
                xyz,
            /* const float color: */
                0.33f);
    }
    #elif T1_RAW_SHADER_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error
    #endif
}
