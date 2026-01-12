#include "T1_renderer.h"

static uint32_t renderer_initialized = false;

void T1_renderer_init(void) {
    renderer_initialized = true;
    
    T1_std_memset(T1_camera, 0, sizeof(T1GPURenderView));
}

inline static void add_opaque_zpolygons_to_workload(
    T1GPUFrame * frame_data)
{
    // for now we assume this always comes 1st
    log_assert(frame_data->verts_size == 0);
    
    int32_t first_opaq_i = (int32_t)frame_data->
        verts_size;
    
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
            T1_zsprite_list->cpu_data[cpu_zp_i].
                deleted ||
            !T1_zsprite_list->cpu_data[cpu_zp_i].
                visible ||
            !T1_zsprite_list->cpu_data[cpu_zp_i].
                committed ||
            T1_zsprite_list->cpu_data[cpu_zp_i].
                alpha_blending_on ||
            T1_zsprite_list->cpu_data[cpu_zp_i].
                bloom_on)
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
        int32_t vert_i =
            T1_objmodel_mesh_summaries[mesh_id].
                vertices_head_i;
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
    
    for (
        uint32_t cam_i = 0;
        cam_i < T1_render_views->size;
        cam_i++)
    {
        for (
            int32_t pass_i = 0;
            pass_i < T1_render_views->cpu[cam_i].
                passes_size;
            pass_i++)
        {
            if (
                T1_render_views->cpu[cam_i].passes[pass_i].type ==
                        T1RENDERPASS_DIAMOND_ALPHA)
            {
                T1_render_views->cpu[cam_i].
                    passes[pass_i].vert_i =
                        first_opaq_i;
                T1_render_views->cpu[cam_i].
                    passes[pass_i].verts_size =
                        (int32_t)frame_data->
                            verts_size;
            }
        }
    }
}

inline static void add_alphablending_zpolygons_to_workload(
    T1GPUFrame * frame_data)
{
    int32_t first_alpha_i = (int32_t)frame_data->
        verts_size;
    
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
            !T1_zsprite_list->cpu_data[cpu_zp_i].alpha_blending_on ||
            T1_zsprite_list->cpu_data[cpu_zp_i].
                bloom_on)
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
    
    for (
        uint32_t cam_i = 0;
        cam_i < T1_render_views->size;
        cam_i++)
    {
        for (
            int32_t pass_i = 0;
            pass_i < T1_render_views->cpu[cam_i].
                passes_size;
            pass_i++)
        {
            if (
                T1_render_views->cpu[cam_i].passes[pass_i].type ==
                        T1RENDERPASS_ALPHA_BLEND)
            {
                T1_render_views->cpu[cam_i].
                    passes[pass_i].vert_i =
                        first_alpha_i;
                T1_render_views->cpu[cam_i].
                    passes[pass_i].verts_size =
                        (int32_t)frame_data->
                            verts_size;
            }
        }
    }
}

inline static void
    add_bloom_zpolygons_to_workload(
        T1GPUFrame * frame_data)
{
    int32_t first_bloom_i = (int32_t)frame_data->
        verts_size;
    
    // Copy all vertices that do use bloom
    for (
        int32_t cpu_zp_i = 0;
        cpu_zp_i < (int32_t)T1_zsprite_list->size;
        cpu_zp_i++)
    {
        if (
            T1_zsprite_list->cpu_data[cpu_zp_i].
                deleted ||
            !T1_zsprite_list->cpu_data[cpu_zp_i].
                visible ||
            !T1_zsprite_list->cpu_data[cpu_zp_i].
                committed ||
            T1_zsprite_list->cpu_data[cpu_zp_i].
                alpha_blending_on ||
            !T1_zsprite_list->cpu_data[cpu_zp_i].
                bloom_on)
        {
            continue;
        }
        
        int32_t mesh_id =
            T1_zsprite_list->cpu_data[cpu_zp_i].
                mesh_id;
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
            frame_data->verts[frame_data->verts_size].
                locked_vertex_i = vert_i;
            frame_data->verts[frame_data->verts_size].
                polygon_i = cpu_zp_i;
            frame_data->verts_size += 1;
            log_assert(
                frame_data->verts_size <
                    MAX_VERTICES_PER_BUFFER);
        }
    }
    
    for (
        uint32_t cam_i = 0;
        cam_i < T1_render_views->size;
        cam_i++)
    {
        for (
            int32_t pass_i = 0;
            pass_i < T1_render_views->cpu[cam_i].
                passes_size;
            pass_i++)
        {
            if (
                T1_render_views->cpu[cam_i].passes[pass_i].type ==
                        T1RENDERPASS_BLOOM)
            {
                T1_render_views->cpu[cam_i].
                    passes[pass_i].vert_i =
                        first_bloom_i;
                T1_render_views->cpu[cam_i].
                    passes[pass_i].verts_size =
                        (int32_t)frame_data->
                            verts_size;
            }
        }
    }
}

static void construct_projection_matrix(void) {
    
    T1GPUProjectConsts * p = NULL;
    
    T1_linal_float4x4 proj;
    
    // Temporary: we get rendering working again
    // by making sure we're handling the easy case first
    log_assert(T1_render_views->cpu[0].width ==
        T1_global->window_width);
    log_assert(T1_render_views->cpu[0].height ==
        T1_global->window_height);
    
    for (
        uint32_t i = 0;
        i < T1_RENDER_VIEW_CAP;
        i++)
    {
        T1GPURenderView * rv = &T1_render_views->gpu[i];
        
        rv->write_to_shadow_maps =
            T1_render_views->cpu[i].write_type ==
                T1RENDERVIEW_WRITE_DEPTH;
        
        rv->read_from_shadow_maps =
            T1_render_views->cpu[i].write_type ==
                T1RENDERVIEW_WRITE_RENDER_TARGET ||
                T1RENDERVIEW_WRITE_RGBA;
        
        const float w =
            (float)T1_render_views->cpu[i].width;
        const float h =
            (float)T1_render_views->cpu[i].height;
        
        const float vertical_fov_degrees = 75.0f;
        const float zn = 0.1f;
        const float zf = 6.0f;
        
        float ar = w / h;
        
        float half_fov_rad = (vertical_fov_degrees * 0.5f) * (3.14159265359f / 180.0f);
        float f = 1.0f / tanf(half_fov_rad);
        
        float x_scale = f / ar;
        float y_scale = f;
        
        T1_linal_float4x4_construct(&proj,
            x_scale, 0.0f, 0.0f, 0.0f,
            0.0f, y_scale, 0.0f, 0.0f,
            0.0f, 0.0f, zf / (zf-zn), -zf*zn/(zf-zn),
            0.0f, 0.0f, 1.0f, 0.0f
        );
        
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
        rv_i < T1_render_views->size;
        rv_i++)
    {
        T1GPURenderView * rv_gpu =
            T1_render_views->gpu + rv_i;
        T1CPURenderView * rv_cpu =
            T1_render_views->cpu + rv_i;
        
        T1_linal_float4x4_construct_identity(
            &result);
        
        T1_linal_float4x4_construct_xyz_rotation(
            &next,
            -rv_cpu->xyz_angle[0],
            -rv_cpu->xyz_angle[1],
            -rv_cpu->xyz_angle[2]);
        
        T1_linal_float4x4_mul_float4x4_inplace(
            &result,
            &next);
        
        T1_linal_float4x4_construct(
            &next,
            1.0f, 0.0f, 0.0f, -rv_cpu->xyz[0],
            0.0f, 1.0f, 0.0f, -rv_cpu->xyz[1],
            0.0f, 0.0f, 1.0f, -rv_cpu->xyz[2],
            0.0f, 0.0f, 0.0f, 1.0f);
        
        T1_linal_float4x4_mul_float4x4_inplace(
            &result, &next);
        
        T1_std_memcpy(
            rv_gpu->v_4x4 + 0,
            result.rows[0].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            rv_gpu->v_4x4 + 4,
            result.rows[1].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            rv_gpu->v_4x4 + 8,
            result.rows[2].data,
            sizeof(float) * 4);
        T1_std_memcpy(
            rv_gpu->v_4x4 + 12,
            result.rows[3].data,
            sizeof(float) * 4);
        
        T1_linal_float4x4_extract_float3x3(
            &result, 3, 3, &view_3x3);
        
        T1_linal_float3x3_inverse_transpose_inplace(
            &view_3x3);
        T1_std_memcpy(
            rv_gpu->normv_3x3 + 0,
            view_3x3.rows[0].data,
            sizeof(float) * 3);
        T1_std_memcpy(
            rv_gpu->normv_3x3 + 3,
            view_3x3.rows[1].data,
            sizeof(float) * 3);
        T1_std_memcpy(
            rv_gpu->normv_3x3 + 6,
            view_3x3.rows[2].data,
            sizeof(float) * 3);
    }
}

#if 0
static void construct_light_matrices(
    T1GPUFrame * frame_data)
{
    T1_linal_float4x4 mat_view;
    T1_linal_float4x4_construct_from_ptr(
        &mat_view,
        T1_render_views->gpu[0].v_4x4);
    
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
    }
}
#endif

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
        log_append(
            "ERROR: platform layer didnt "
            "pass recipients\n");
        return;
    }
    
    log_assert(T1_zsprite_list->size <
        MAX_ZSPRITES_PER_BUFFER);
    
    #if T1_PROFILER_ACTIVE == T1_ACTIVE
    T1_profiler_start("construct render matrices");
    #elif T1_PROFILER_ACTIVE == T1_INACTIVE
    #else
    #error "T1_PROFILER_ACTIVE undefined"
    #endif
    
    T1_zlight_update_all_attached_render_views();
    
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
        frame_data->zsprite_list->size <=
            T1_zsprite_list->size);
    
    log_assert(
        T1_zsprite_list->size <
            MAX_ZSPRITES_PER_BUFFER);
    log_assert(
        frame_data->zsprite_list->size <
            MAX_ZSPRITES_PER_BUFFER);
    
    *frame_data->postproc_consts =
        T1_global->postproc_consts;
    
    add_opaque_zpolygons_to_workload(frame_data);
    
    #if T1_BLENDING_SHADER_ACTIVE == T1_ACTIVE
    add_alphablending_zpolygons_to_workload(frame_data);
    #elif T1_BLENDING_SHADER_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    add_bloom_zpolygons_to_workload(frame_data);
    
    #if T1_PARTICLES_ACTIVE == T1_ACTIVE
    
    #if T1_PROFILER_ACTIVE == T1_ACTIVE
    T1_profiler_start(
        "T1_particle_add_all_to_frame_data()");
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
    
    // construct_light_matrices(frame_data);
    
    #if T1_FRAME_ANIM_ACTIVE == T1_ACTIVE
    T1_frame_anim_apply_all(frame_data);
    #elif T1_FRAME_ANIM_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    frame_data->render_views_size =
        T1_render_views->size;
    
    log_assert(frame_data->render_views != NULL);
    T1_std_memcpy(
        frame_data->render_views,
        T1_render_views->gpu,
        sizeof(T1GPURenderView) * T1_RENDER_VIEW_CAP);
    log_assert(frame_data->render_views != NULL);
}
