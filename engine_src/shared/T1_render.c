#include "T1_render.h"

static uint32_t T1_render_active = false;

void T1_render_init(void) {
    T1_render_active = true;
    
    T1_std_memset(
        T1_camera,
        0,
        sizeof(T1GPURenderView));
}

static void
construct_projection_matrix(void)
{
    T1_linal_float4x4 proj;
    
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
        const float zf = T1_ZFAR;
        
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

void T1_render_update(
    T1GPUFrame * frame_data,
    uint64_t elapsed_us)
{
    (void)elapsed_us;
    
    if (T1_render_active != true) {
        T1_log_append("renderer not initialized, aborting...\n");
        return;
    }
    
    if (
        frame_data == NULL ||
        frame_data->verts == NULL)
    {
        T1_log_append(
            "ERROR: platform layer didnt "
            "pass recipients\n");
        return;
    }
    
    frame_data->zsprite_list->size = 0;
    
    T1_zlight_update_all_attached_render_views();
    
    construct_view_matrix();
    
    construct_projection_matrix();
    
    T1_zsprite_copy_to_frame_data(
        frame_data->zsprite_list->polygons,
        frame_data->id_pairs,
        &frame_data->zsprite_list->size);
    
    T1_zsprite_construct_model_and_normal_matrices(
        frame_data);
    
    T1_texquad_copy_to_frame_data(
        frame_data->flat_tex_quads,
        &frame_data->flat_tex_quads_size);
        
    *frame_data->postproc_consts =
        T1_global->postproc_consts;
    
    T1_zsprite_add_opaque_zpolygons_to_workload(frame_data);
    T1_log_assert(frame_data->zsprite_list->size <
        T1_ZSPRITES_CAP);
    
    #if T1_BLENDING_SHADER_ACTIVE == T1_ACTIVE
    T1_zsprite_add_alphablending_zpolygons_to_workload(frame_data);
    T1_log_assert(frame_data->zsprite_list->size <
        T1_ZSPRITES_CAP);
    #elif T1_BLENDING_SHADER_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
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
                        T1RENDERPASS_OUTLINES)
            {
                T1_render_views->cpu[cam_i].
                    passes[pass_i].vert_i = 0;
                T1_render_views->cpu[cam_i].
                    passes[pass_i].verts_size =
                        (int32_t)frame_data->
                            verts_size;
            }
        }
    }
    
    T1_zsprite_add_bloom_zpolygons_to_workload(
        frame_data);
    T1_log_assert(frame_data->zsprite_list->size <
        T1_ZSPRITES_CAP);
    
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
    T1_log_assert(frame_data->zsprite_list->size <
        T1_ZSPRITES_CAP);
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
                T1_render_views->cpu[cam_i].
                    passes[pass_i].type ==
                        T1RENDERPASS_BILLBOARDS)
            {
                T1_render_views->cpu[cam_i].
                    passes[pass_i].vert_i = 0;
                T1_render_views->cpu[cam_i].
                    passes[pass_i].verts_size =
                        (int32_t)frame_data->
                            flat_bb_quads_size;
            }
            
            if (
                T1_render_views->cpu[cam_i].passes[pass_i].type ==
                        T1RENDERPASS_FLAT_TEXQUADS)
            {
                T1_render_views->cpu[cam_i].
                    passes[pass_i].vert_i = 0;
                T1_render_views->cpu[cam_i].
                    passes[pass_i].verts_size =
                        (int32_t)frame_data->
                            flat_tex_quads_size;
            }
        }
    }
    
    // construct_light_matrices(frame_data);
    
    #if T1_FRAME_ANIM_ACTIVE == T1_ACTIVE
    T1_frame_anim_apply_all(frame_data);
    #elif T1_FRAME_ANIM_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    frame_data->render_views_size =
        T1_render_views->size;
    
    T1_log_assert(frame_data->render_views != NULL);
    T1_std_memcpy(
        frame_data->render_views,
        T1_render_views->gpu,
        sizeof(T1GPURenderView) * T1_RENDER_VIEW_CAP);
    T1_log_assert(frame_data->render_views != NULL);
}
