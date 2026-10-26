#include "T1_render.h"

#include "T1_std.h"
#include "T1_assert.h"
#include "T1_linalg3d.h"
#include "T1_global.h"
#include "T1_simd.h"
#include "T1_os.h"
#include "T1_types_gpucpu.h"
#include "T1_texquad.h"
#include "T1_zsprite.h"
#include "T1_particle.h"
#include "T1_io.h"
#include "T1_zlight.h"
#include "T1_render_view.h"

static u32 T1_render_active = 0;

void T1_render_init(void) {
    T1_render_active = 1;
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
        
        rv->write_to_shadow_maps = 0;
        for (u32 zl_i = 0; zl_i < T1_zlights_size; zl_i++) {
            if (
                !T1_zlights[zl_i].deleted &&
                T1_zlights[zl_i].u32s.shadow_cam ==
                    T1_render_views->cpu[i].T1_id)
            {
                rv->write_to_shadow_maps = 1;
            }
        }
        
        rv->visible_layers = T1_render_views->cpu[i].visible_layers;
        
        const T1GPUProjectConsts * pjc = &T1_render_views->cpu[i].project;
        const f32 zn = pjc->znear;
        const f32 zf = pjc->zfar;
        
        f32 x_scale = pjc->field_of_view_modifier / pjc->aspect_ratio;
        f32 y_scale = pjc->field_of_view_modifier;
        
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
            -rv_cpu->f32s.angle_xyz[0],
            -rv_cpu->f32s.angle_xyz[1],
            -rv_cpu->f32s.angle_xyz[2]);
        
        T1_linal_f32x4x4_mul_f32x4x4_inplace(
            &result,
            &next);
        
        T1_linal_f32x4x4_construct(
            &next,
            1.0f, 0.0f, 0.0f, -rv_cpu->f32s.xyz[0],
            0.0f, 1.0f, 0.0f, -rv_cpu->f32s.xyz[1],
            0.0f, 0.0f, 1.0f, -rv_cpu->f32s.xyz[2],
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
    
    if (T1_render_active != 1) {
        return;
    }
    
    if (f == NULL || f->verts == NULL)
    {
        return;
    }
    
    for (u32 rv_i = 0; rv_i < T1_render_views->size; rv_i++) {
        T1CPURenderView * rv = T1_render_views->cpu + rv_i;
        if (rv->clamped_to_T1_id != T1_ID_NONE) {
            u32 T1_id = rv->clamped_to_T1_id;
            
            T1_zsprite_get_pos_xyz(
                T1_id,
                &rv->f32s.dest_xyz[0],
                &rv->f32s.dest_xyz[1],
                &rv->f32s.dest_xyz[2]);
            rv->f32s.dest_xyz[2] -= 0.50f;
            rv->f32s.min_xyz[0] = rv->f32s.dest_xyz[0];
            rv->f32s.min_xyz[1] = rv->f32s.dest_xyz[1];
            rv->f32s.min_xyz[2] = rv->f32s.dest_xyz[2];
            rv->f32s.max_xyz[0] = rv->f32s.dest_xyz[0];
            rv->f32s.max_xyz[1] = rv->f32s.dest_xyz[1];
            rv->f32s.max_xyz[2] = rv->f32s.dest_xyz[2];
        }
    }
    
    f->zsprite_list->size = 0;
    for (u32 i = 0; i < T1_VERT_RANGES_SIZE; i++) {
        f->vert_ranges[i].vert_i = 0;
        f->vert_ranges[i].verts_size = 0;
    }
    
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
    
    *f->frame_consts = T1_global->frame_consts;
    
    T1_zsprite_add_opaque_zpolygons_to_workload(f);
    T1_assert(f->zsprite_list->size < T1_ZSPRITES_CAP);
    
    #if T1_BLENDING_SHADER_ACTIVE == T1_ACTIVE
    T1_zsprite_add_alphablending_zpolygons_to_workload(f);
    T1_assert(f->zsprite_list->size <
        T1_ZSPRITES_CAP);
    #elif T1_BLENDING_SHADER_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_zsprite_add_bloom_zpolygons_to_workload(f);
    T1_assert(f->zsprite_list->size <
        T1_ZSPRITES_CAP);
    
    #if T1_PARTICLES_ACTIVE == T1_ACTIVE
    
    T1_particle_add_all_to_frame_data(
        /* GPUDataForSingleFrame * frame_data: */
            f,
        /* u64 elapsed_us: */
            elapsed_us);
    T1_assert(f->zsprite_list->size <
        T1_ZSPRITES_CAP);
    
    #elif T1_PARTICLES_ACTIVE == T1_INACTIVE
    // Pass
    #else
    #error
    #endif
    
    f->render_views_size = T1_render_views->size;
    
    T1_assert(f->render_views != NULL);
    T1_std_memcpy(
        f->render_views,
        T1_render_views->gpu,
        sizeof(T1GPURenderView) * T1_RENDER_VIEW_CAP);
    T1_assert(f->render_views != NULL);
}
