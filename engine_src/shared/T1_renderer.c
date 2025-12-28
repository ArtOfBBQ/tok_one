#include "T1_renderer.h"

static uint32_t renderer_initialized = false;

void T1_renderer_init(void) {
    renderer_initialized = true;
    
    T1_std_memset(T1_camera, 0, sizeof(T1GPURenderView));
}

inline static void add_alphablending_zpolygons_to_workload(
    T1GPUFrame * frame_data)
{
    frame_data->first_alphablend_i = frame_data->verts_size;
    
    // Copy all vertices that do use alpha blending
    for (
        int32_t cpu_zp_i = 0;
        cpu_zp_i < (int32_t)T1_zsprite_list->size;
        cpu_zp_i++)
    {
        if (
            T1_zsprite_list->cpu_data[cpu_zp_i].deleted ||
            !T1_zsprite_list->cpu_data[cpu_zp_i].visible ||
            !T1_zsprite_list->cpu_data[cpu_zp_i].committed ||
            !T1_zsprite_list->cpu_data[cpu_zp_i].alpha_blending_on)
        {
            continue;
        }
        
        int32_t mesh_id = T1_zsprite_list->cpu_data[cpu_zp_i].mesh_id;
        log_assert(mesh_id >= 0);
        log_assert(mesh_id < (int32_t)T1_objmodel_mesh_summaries_size);
        
        int32_t vert_tail_i =
            T1_objmodel_mesh_summaries[mesh_id].vertices_head_i +
                T1_objmodel_mesh_summaries[mesh_id].vertices_size;
        assert(vert_tail_i < MAX_VERTICES_PER_BUFFER);
        
        for (
            int32_t vert_i = T1_objmodel_mesh_summaries[mesh_id].vertices_head_i;
            vert_i < vert_tail_i;
            vert_i += 1)
        {
            frame_data->verts[frame_data->verts_size].locked_vertex_i =
                vert_i;
            frame_data->verts[frame_data->verts_size].polygon_i =
                cpu_zp_i;
            frame_data->verts_size += 1;
            log_assert(
                frame_data->verts_size <
                    MAX_VERTICES_PER_BUFFER);
        }
    }
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
        cpu_zp_i < (int32_t)T1_zsprite_list->size;
        cpu_zp_i++)
    {
        if (
            T1_zsprite_list->cpu_data[cpu_zp_i].deleted ||
            !T1_zsprite_list->cpu_data[cpu_zp_i].visible ||
            !T1_zsprite_list->cpu_data[cpu_zp_i].committed ||
            T1_zsprite_list->cpu_data[cpu_zp_i].alpha_blending_on)
        {
            continue;
        }
        
        int32_t mesh_id = T1_zsprite_list->
            cpu_data[cpu_zp_i].mesh_id;
        log_assert(mesh_id >= 0);
        log_assert(mesh_id < (int32_t)T1_objmodel_mesh_summaries_size);
        
        int32_t vert_tail_i =
            T1_objmodel_mesh_summaries[mesh_id].
                vertices_head_i +
                    T1_objmodel_mesh_summaries[mesh_id].
                        vertices_size;
        log_assert(
            vert_tail_i < MAX_VERTICES_PER_BUFFER);
        
        /*
        We are free to overflow the vertices buffer, since its end is not
        in use yet anyway.
        */
        int32_t vert_i = T1_objmodel_mesh_summaries[mesh_id].vertices_head_i;
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
    
    T1GPUProjectConsts * p =
        &T1_global->project_consts;
    
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
    
    for (
        uint32_t i = 0;
        i < T1_render_views_size;
        i++)
    {
        T1GPURenderView * rv = T1_render_views + i;
        
        T1_std_memcpy(
            rv->p_4x4 + 0,
            proj.rows[0].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            rv->p_4x4 + 4,
            proj.rows[1].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            rv->p_4x4 + 8,
            proj.rows[2].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            rv->p_4x4 + 12,
            proj.rows[3].data,
            sizeof(float) * 4);
    }
}

static void construct_view_matrix(void) {
    
    T1_linal_float4x4 result;
    T1_linal_float4x4 next;
    T1_linal_float3x3 view_3x3;
    
    for (
        uint32_t rv_i = 0;
        rv_i < T1_render_views_size;
        rv_i++)
    {
        T1GPURenderView * rv = T1_render_views + rv_i;
        
        rv->xyz_cosangle[0] =
            cosf(T1_camera->xyz_angle[0]);
        rv->xyz_cosangle[1] =
            cosf(T1_camera->xyz_angle[1]);
        rv->xyz_cosangle[2] =
            cosf(T1_camera->xyz_angle[2]);
        rv->xyz_sinangle[0] =
            sinf(T1_camera->xyz_angle[0]);
        rv->xyz_sinangle[1] =
            sinf(T1_camera->xyz_angle[1]);
        rv->xyz_sinangle[2] =
            sinf(T1_camera->xyz_angle[2]);
        
        T1_linal_float4x4_construct_identity(
            &result);
        
        T1_linal_float4x4_construct_xyz_rotation(
            &next,
            -rv->xyz_angle[0],
            -rv->xyz_angle[1],
            -rv->xyz_angle[2]);
        
        T1_linal_float4x4_mul_float4x4_inplace(
            &result,
            &next);
        
        T1_linal_float4x4_construct(
            &next,
            1.0f, 0.0f, 0.0f, -rv->xyz[0],
            0.0f, 1.0f, 0.0f, -rv->xyz[1],
            0.0f, 0.0f, 1.0f, -rv->xyz[2],
            0.0f, 0.0f, 0.0f, 1.0f);
        
        T1_linal_float4x4_mul_float4x4_inplace(
            &result, &next);
        
        T1_std_memcpy(
            rv->v_4x4 + 0,
            result.rows[0].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            rv->v_4x4 + 4,
            result.rows[1].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            rv->v_4x4 + 8,
            result.rows[2].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            rv->v_4x4 + 12,
            result.rows[3].data,
            sizeof(float) * 4);
        
        T1_linal_float4x4_extract_float3x3(
            &result, 3, 3, &view_3x3);
        
        T1_linal_float3x3_inverse_transpose_inplace(
            &view_3x3);
        T1_std_memcpy(
            rv->normv_3x3 + 0,
            view_3x3.rows[0].data,
            sizeof(float) * 3);
        T1_std_memcpy(
            rv->normv_3x3 + 3,
            view_3x3.rows[1].data,
            sizeof(float) * 3);
        T1_std_memcpy(
            rv->normv_3x3 + 6,
            view_3x3.rows[2].data,
            sizeof(float) * 3);
    }
}

static void construct_light_matrices(
    T1GPUFrame * frame_data)
{
    T1_linal_float4x4 mat_view;
    T1_linal_float4x4_construct_from_ptr(
        &mat_view,
        T1_camera->v_4x4);
    
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
        light_i < frame_data->postproc_consts->lights_size;
        light_i++)
    {
        T1_linal_float4 light_world;
        // reminder: frame data includes an offset
        light_world.data[0] =
            frame_data->lights[light_i].xyz[0];
        light_world.data[1] =
            frame_data->lights[light_i].xyz[1];
        light_world.data[2] =
            frame_data->lights[light_i].xyz[2];
        light_world.data[3] = 1.0f;
        
        T1_linal_float4 view_pos =
            T1_linal_float4x4_mul_float4(
                &mat_view,
                light_world);
        
        frame_data->lights[light_i].
            viewspace_xyz[0] = view_pos.data[0];
        frame_data->lights[light_i].
            viewspace_xyz[1] = view_pos.data[1];
        frame_data->lights[light_i].
            viewspace_xyz[2] = view_pos.data[2];
        
        // Next, we want to transform from camera view to light view
        T1_linal_float4x4_construct_xyz_rotation(
            &a_4x4,
            -frame_data->lights[light_i].
                angle_xyz[0],
            -frame_data->lights[light_i].
                angle_xyz[1],
            -frame_data->lights[light_i].
                angle_xyz[2]);
        
        T1_linal_float4x4_construct(
            &b_4x4,
            1.0f, 0.0f, 0.0f, -frame_data->lights[light_i].xyz[0],
            0.0f, 1.0f, 0.0f, -frame_data->lights[light_i].xyz[1],
            0.0f, 0.0f, 1.0f, -frame_data->lights[light_i].xyz[2],
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
    // T1_linal_float3x3 view3x3;
    
    for (
        uint32_t i = 0;
        i < T1_zsprite_list->size;
        i++)
    {
        
        T1CPUzSpriteSimdStats * s =
            &T1_zsprite_list->cpu_data[i].
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
            T1_zsprite_list->gpu_data[i].
                m_4x4 + 0,
            result.rows[0].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            T1_zsprite_list->gpu_data[i].
                m_4x4 + 4,
            result.rows[1].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            T1_zsprite_list->gpu_data[i].
                m_4x4 + 8,
            result.rows[2].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            T1_zsprite_list->gpu_data[i].
                m_4x4 + 12,
            result.rows[3].data,
            sizeof(float) * 4);
        
        // Next: transforming normals
        // store topleft 3x3 of "model to world"
        // matrix in model3x3
        T1_linal_float4x4_extract_float3x3(
            /* const T1_linal_float4x4 * in: */
                &result,
            /* const int omit_row_i: */
                3,
            /* const int omit_col_i: */
                3,
            /* T1_linal_float3x3 * out: */
                &model3x3);
        
        // inverse transpose the topleft 3x3
        T1_linal_float3x3_inverse_transpose_inplace(&model3x3);
        
        // store as the "normal to world" matrix
        T1_zsprite_list->gpu_data[i].
            norm_3x3[0] = model3x3.rows[0].data[0];
        T1_zsprite_list->gpu_data[i].
            norm_3x3[1] = model3x3.rows[0].data[1];
        T1_zsprite_list->gpu_data[i].
            norm_3x3[2] = model3x3.rows[0].data[2];
        T1_zsprite_list->gpu_data[i].
            norm_3x3[3] = model3x3.rows[1].data[0];
        T1_zsprite_list->gpu_data[i].
            norm_3x3[4] = model3x3.rows[1].data[1];
        T1_zsprite_list->gpu_data[i].
            norm_3x3[5] = model3x3.rows[1].data[2];
        T1_zsprite_list->gpu_data[i].
            norm_3x3[6] = model3x3.rows[2].data[0];
        T1_zsprite_list->gpu_data[i].
            norm_3x3[7] = model3x3.rows[2].data[1];
        T1_zsprite_list->gpu_data[i].
            norm_3x3[8] = model3x3.rows[2].data[2];
    }
}

void T1_renderer_hardware_render(
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
    
    log_assert(T1_zsprite_list->size < MAX_ZSPRITES_PER_BUFFER);
    
    #if T1_PROFILER_ACTIVE == T1_ACTIVE
    T1_profiler_start("construct render matrices");
    #elif T1_PROFILER_ACTIVE == T1_INACTIVE
    #else
    #error "T1_PROFILER_ACTIVE undefined"
    #endif
    construct_view_matrix();
    
    construct_projection_matrix();
    
    construct_model_and_normal_matrices();
    #if T1_PROFILER_ACTIVE == T1_ACTIVE
    T1_profiler_end("construct render matrices");
    #elif T1_PROFILER_ACTIVE == T1_INACTIVE
    #else
    #error "T1_PROFILER_ACTIVE undefined"
    #endif
    
    T1_std_memcpy(
        /* void * dest: */
            frame_data->zsprite_list->polygons,
        /* const void * src: */
            T1_zsprite_list->gpu_data,
        /* size_t n: */
            sizeof(T1GPUzSprite) * T1_zsprite_list->size);
    frame_data->zsprite_list->size = T1_zsprite_list->size;
    
    log_assert(
        frame_data->zsprite_list->size <= T1_zsprite_list->size);
    log_assert(
        T1_zsprite_list->size < MAX_ZSPRITES_PER_BUFFER);
    log_assert(
        frame_data->zsprite_list->size < MAX_ZSPRITES_PER_BUFFER);
    
    *frame_data->postproc_consts =
        T1_global->postproc_consts;
    
    add_opaque_zpolygons_to_workload(frame_data);
    
    #if T1_BLENDING_SHADER_ACTIVE == T1_ACTIVE
    add_alphablending_zpolygons_to_workload(frame_data);
    #elif T1_BLENDING_SHADER_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    #if T1_PARTICLES_ACTIVE == T1_ACTIVE
    
    #if T1_PROFILER_ACTIVE == T1_ACTIVE
    T1_profiler_start("T1_particle_add_all_to_frame_data()");
    #elif T1_PROFILER_ACTIVE == T1_INACTIVE
    #else
    #error "T1_PROFILER_ACTIVE undefined"
    #endif
    T1_particle_add_all_to_frame_data(
        /* GPUDataForSingleFrame * frame_data: */
            frame_data,
        /* uint64_t elapsed_us: */
            elapsed_us);
    #if T1_PROFILER_ACTIVE == T1_ACTIVE
    T1_profiler_end(
        "T1_particle_add_all_to_frame_data()");
    #elif T1_PROFILER_ACTIVE == T1_INACTIVE
    #else
    #error "T1_PROFILER_ACTIVE undefined"
    #endif
    
    #elif T1_PARTICLES_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error
    #endif
    
    construct_light_matrices(frame_data);
    
    #if T1_FRAME_ANIM_ACTIVE == T1_ACTIVE
    T1_frame_anim_apply_all(frame_data);
    #elif T1_FRAME_ANIM_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    frame_data->render_views_size =
        T1_render_views_size;
    for (
        uint32_t rv_i = 0;
        rv_i < T1_render_views_size;
        rv_i++)
    {
        log_assert(frame_data->render_views[rv_i] != NULL);
        T1_std_memcpy(
            frame_data->render_views[rv_i],
            T1_render_views + rv_i,
            sizeof(T1GPURenderView));
        log_assert(frame_data->render_views[rv_i] != NULL);
    }
}
