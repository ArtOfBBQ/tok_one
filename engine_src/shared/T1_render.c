#include "T1_render.h"

#include "T1_std.h"
#include "T1_log.h"
#include "T1_profiler.h"
#include "T1_linalg3d.h"
#include "T1_global.h"
#include "T1_simd.h"
#include "T1_platform_layer.h"
#include "T1_types_gpucpu.h"
#include "T1_texquad.h"
#include "T1_zsprite.h"
#include "T1_particle.h"
#include "T1_io.h"
#include "T1_zlight.h"
#include "T1_frame_anim.h"
#include "T1_render_view.h"

static u32 T1_render_active = false;

void T1_render_init(void) {
    T1_render_active = true;
}

static void
construct_projection_matrix(void)
{
    T1_linal_f32x4x4 proj;
    
    for (
        u32 i = 0;
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
        
        const f32 w = (f32)T1_render_views->cpu[i].width;
        const f32 h = (f32)T1_render_views->cpu[i].height;
        
        const f32 vertical_fov_degrees = 75.0f;
        const f32 zn = 0.1f;
        const f32 zf = T1_ZFAR;
        
        f32 ar = w / h;
        
        f32 half_fov_rad = (vertical_fov_degrees * 0.5f) * (3.14159265359f / 180.0f);
        f32 f = 1.0f / tanf(half_fov_rad);
        
        f32 x_scale = f / ar;
        f32 y_scale = f;
        
        T1_linal_f32x4x4_construct(&proj,
            x_scale, 0.0f, 0.0f, 0.0f,
            0.0f, y_scale, 0.0f, 0.0f,
            0.0f, 0.0f, zf / (zf-zn), -zf*zn/(zf-zn),
            0.0f, 0.0f, 1.0f, 0.0f
        );
        
        T1_std_memcpy(
            rv->p_4x4 + 0,
            proj.rows[0].data,
            sizeof(f32) * 4);
        T1_std_memcpy(
            rv->p_4x4 + 4,
            proj.rows[1].data,
            sizeof(f32) * 4);
        T1_std_memcpy(
            rv->p_4x4 + 8,
            proj.rows[2].data,
            sizeof(f32) * 4);
        T1_std_memcpy(
            rv->p_4x4 + 12,
            proj.rows[3].data,
            sizeof(f32) * 4);
    }
}

static void construct_view_matrix(void) {
    
    T1_linal_f32x4x4 result;
    T1_linal_f32x4x4 next;
    T1_linal_f32x3x3 view_3x3;
    
    for (
        u32 rv_i = 0;
        rv_i < T1_render_views->size;
        rv_i++)
    {
        T1GPURenderView * rv_gpu = T1_render_views->gpu + rv_i;
        T1CPURenderView * rv_cpu = T1_render_views->cpu + rv_i;
        
        T1_linal_f32x4x4_construct_identity(
            &result);
        
        T1_linal_f32x4x4_construct_xyz_rotation(
            &next,
            -rv_cpu->angle_xyz[0],
            -rv_cpu->angle_xyz[1],
            -rv_cpu->angle_xyz[2]);
        
        T1_linal_f32x4x4_mul_f32x4x4_inplace(
            &result,
            &next);
        
        T1_linal_f32x4x4_construct(
            &next,
            1.0f, 0.0f, 0.0f, -rv_cpu->xyz[0],
            0.0f, 1.0f, 0.0f, -rv_cpu->xyz[1],
            0.0f, 0.0f, 1.0f, -rv_cpu->xyz[2],
            0.0f, 0.0f, 0.0f, 1.0f);
        
        T1_linal_f32x4x4_mul_f32x4x4_inplace(&result, &next);
        
        T1_std_memcpy(
            rv_gpu->v_4x4 + 0,
            result.rows[0].data,
            sizeof(f32) * 4);
        T1_std_memcpy(
            rv_gpu->v_4x4 + 4,
            result.rows[1].data,
            sizeof(f32) * 4);
        T1_std_memcpy(
            rv_gpu->v_4x4 + 8,
            result.rows[2].data,
            sizeof(f32) * 4);
        T1_std_memcpy(
            rv_gpu->v_4x4 + 12,
            result.rows[3].data,
            sizeof(f32) * 4);
        
        T1_linal_f32x4x4_extract_f32x3x3(
            &result, 3, 3, &view_3x3);
        
        T1_linal_f32x3x3_inverse_transpose_inplace(
            &view_3x3);
        T1_std_memcpy(
            rv_gpu->normv_3x3 + 0,
            view_3x3.rows[0].data,
            sizeof(f32) * 3);
        T1_std_memcpy(
            rv_gpu->normv_3x3 + 3,
            view_3x3.rows[1].data,
            sizeof(f32) * 3);
        T1_std_memcpy(
            rv_gpu->normv_3x3 + 6,
            view_3x3.rows[2].data,
            sizeof(f32) * 3);
    }
}

void T1_render_update(
    T1GPUFrame * f,
    u64 elapsed_us)
{
    (void)elapsed_us;
    
    if (T1_render_active != true) {
        T1_log_append(
            "renderer not initialized, aborting...\n");
        return;
    }
    
    if (f == NULL || f->verts == NULL)
    {
        T1_log_append(
            "ERROR: platform layer didnt "
            "pass recipients\n");
        return;
    }
    
    for (u32 rv_i = 0; rv_i < T1_render_views->size; rv_i++) {
        T1CPURenderView * rv = T1_render_views->cpu + rv_i;
        if (rv->clamped_to_T1_id >= 0) {
            s32 T1_id = rv->clamped_to_T1_id;
            
            T1_zsprite_get_pos_xyz(
                T1_id,
                &rv->dest_xyz[0],
                &rv->dest_xyz[1],
                &rv->dest_xyz[2]);
            rv->dest_xyz[2] -= 0.50f;
            rv->min_xyz[0] = rv->dest_xyz[0];
            rv->min_xyz[1] = rv->dest_xyz[1];
            rv->min_xyz[2] = rv->dest_xyz[2];
            rv->max_xyz[0] = rv->dest_xyz[0];
            rv->max_xyz[1] = rv->dest_xyz[1];
            rv->max_xyz[2] = rv->dest_xyz[2];
        }
    }
    
    f->zsprite_list->size = 0;
    
    T1_zlight_update_all_attached_render_views();
    
    construct_view_matrix();
    
    construct_projection_matrix();
    
    T1_zsprite_copy_to_frame_data(
        f->zsprite_list->polygons,
        f->id_pairs,
        &f->zsprite_list->size);
    
    T1_zsprite_construct_model_and_normal_matrices(f);
    
    T1_texquad_copy_to_frame_data(
        f->flat_tex_quads,
        &f->flat_tex_quads_size);
    
    *f->postproc_consts = T1_global->postproc_consts;
    
    T1_zsprite_add_opaque_zpolygons_to_workload(f);
    T1_log_assert(f->zsprite_list->size < T1_ZSPRITES_CAP);
    
    #if T1_BLENDING_SHADER_ACTIVE == T1_ACTIVE
    T1_zsprite_add_alphablending_zpolygons_to_workload(f);
    T1_log_assert(f->zsprite_list->size <
        T1_ZSPRITES_CAP);
    #elif T1_BLENDING_SHADER_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    for (
        u32 cam_i = 0;
        cam_i < T1_render_views->size;
        cam_i++)
    {
        for (
            s32 pass_i = 0;
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
                        (s32)f->verts_size;
            }
        }
    }
    
    T1_zsprite_add_bloom_zpolygons_to_workload(f);
    T1_log_assert(f->zsprite_list->size <
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
            f,
        /* u64 elapsed_us: */
            elapsed_us);
    T1_log_assert(f->zsprite_list->size <
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
        u32 cam_i = 0;
        cam_i < T1_render_views->size;
        cam_i++)
    {
        for (
            s32 pass_i = 0;
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
                        (s32)f->flat_bb_quads_size;
            }
            
            if (
                T1_render_views->cpu[cam_i].passes[pass_i].type ==
                    T1RENDERPASS_FLAT_TEXQUADS)
            {
                T1_render_views->cpu[cam_i].
                    passes[pass_i].vert_i = 0;
                T1_render_views->cpu[cam_i].
                    passes[pass_i].verts_size =
                        (s32)f->flat_tex_quads_size;
            }
        }
    }
    
    f->render_views_size = T1_render_views->size;
    
    T1_log_assert(f->render_views != NULL);
    T1_std_memcpy(
        f->render_views,
        T1_render_views->gpu,
        sizeof(T1GPURenderView) * T1_RENDER_VIEW_CAP);
    T1_log_assert(f->render_views != NULL);
}
