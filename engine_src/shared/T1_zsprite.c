#include "T1_zsprite.h"

#include "T1_simd.h"
#include "T1_mem.h"
#include "T1_log.h"
#include "T1_global.h"
#include "T1_material.h"
#include "T1_mesh_summary.h"
#include "T1_zlight.h"
#include "T1_render_view.h"
#include "T1_linalg3d.h"

T1zSpriteCollection * T1_zsprite_list = NULL;

#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
static void
assert_sanity_check_zsprite_vals(
    T1GPUzSprite * recip_gpu,
    T1CPUzSpriteSimdStats * recip_cpu)
{
    if (recip_gpu) {
        T1_log_assert(recip_gpu->f32s.
            no_camera >= -0.05f);
        T1_log_assert(recip_gpu->f32s.
            no_camera <= 1.05f);
        T1_log_assert(recip_gpu->f32s.
            no_lighting >= -0.05f);
        T1_log_assert(recip_gpu->f32s.
            no_lighting <= 1.05f);
        T1_log_assert(recip_gpu->f32s.
            shadow_strength >= -0.1f);
        T1_log_assert(recip_gpu->f32s.
            shadow_strength <= 1.1f);
        T1_log_assert(recip_gpu->f32s.alpha >= -0.1f);
        T1_log_assert(recip_gpu->f32s.alpha <=  1.1f);
        T1_log_assert(recip_gpu->f32s.bonus_rgb[0] < 3.0f);
        T1_log_assert(recip_gpu->f32s.bonus_rgb[1] < 3.0f);
        T1_log_assert(recip_gpu->f32s.bonus_rgb[2] < 3.0f);
    }
    
    if (recip_cpu) {
        T1_log_assert(recip_cpu->scale_factor >    0.0f);
        T1_log_assert(recip_cpu->scale_factor < 1000.0f);
        T1_log_assert(recip_cpu->mul_xyz[0] > 0.0f);
        T1_log_assert(recip_cpu->mul_xyz[1] > 0.0f);
        T1_log_assert(recip_cpu->mul_xyz[2] > 0.0f);
    }
}

static void
assert_sanity_check_zsprite_vals_by_id(
    const s32 zp_i)
{
    assert_sanity_check_zsprite_vals(
        &T1_zsprite_list->gpu[zp_i],
        &T1_zsprite_list->cpu[zp_i].
            simd_stats);
}
#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#define assert_sanity_check_zsprite_vals(x, y)
#define assert_sanity_check_zsprite_vals_by_id(id)
#else
#error
#endif

void
T1_zsprite_init(void) {
    T1_zsprite_list = (T1zSpriteCollection *)
        T1_mem_malloc_unmanaged(
            sizeof(T1zSpriteCollection));
    T1_zsprite_list->size = 0;
}

void
T1_zsprite_defragment(void) {
    s32 i = 0;
    s32 j = (s32)T1_zsprite_list->size-1;
    
    while (i < j) {
        if (!T1_zsprite_list->cpu[i].deleted) {
            i++;
            continue;
        }
        
        if (T1_zsprite_list->cpu[i].deleted) {
            T1_zsprite_list->cpu[i] = T1_zsprite_list->cpu[j];
            T1_zsprite_list->gpu[i] = T1_zsprite_list->gpu[j];
            T1_zsprite_list->cpu[j].deleted = true;
            j--;
        }
    }
    
    while (
        T1_zsprite_list->size > 0 &&
        T1_zsprite_list->cpu[T1_zsprite_list->size-1].deleted)
    {
        T1_zsprite_list->size -= 1;
    }
}

void
T1_zsprite_fetch_next_noconstruct(
    T1zSpriteRequest * stack_recipient)
{
    stack_recipient->cpu_data = NULL;
    stack_recipient->gpu_data = NULL;
    
    stack_recipient->cpu_data =
        &T1_zsprite_list->cpu[T1_zsprite_list->size];
    stack_recipient->gpu_data =
        &T1_zsprite_list->gpu[T1_zsprite_list->size];
    stack_recipient->cpu_data[T1_zsprite_list->size].
        deleted = false;
    stack_recipient->cpu_data->committed = false;
    
    T1_zsprite_list->size += 1;
    T1_log_assert(T1_zsprite_list->size + 1 <
        T1_ZSPRITES_CAP);
}

void
T1_zsprite_commit(
    T1zSpriteRequest * to_commit)
{
    T1_log_assert(to_commit->cpu_data->mesh_id >= 0);
    T1_log_assert(to_commit->cpu_data->mesh_id <
        (s32)T1_mesh_summary_list_size);
    T1_log_assert(to_commit->cpu_data->mesh_id < T1_MESH_CAP);
    T1_log_assert(
        T1_mesh_summary_list[to_commit->cpu_data->mesh_id].
            vertices_size > 0);
    
    // probably shouldn't be requesting sprites in africa
    T1_log_warn(to_commit->cpu_data->simd_stats.xyz[0] > -100.0f);
    T1_log_warn(to_commit->cpu_data->simd_stats.xyz[1] > -100.0f);
    T1_log_warn(to_commit->cpu_data->simd_stats.xyz[0] <  100.0f);
    T1_log_warn(to_commit->cpu_data->simd_stats.xyz[1] <  100.0f);
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    u32 all_mesh_vertices_tail_i =
        (u32)(
            T1_mesh_summary_list
                [to_commit->cpu_data->mesh_id].vertices_head_i +
            T1_mesh_summary_list
                [to_commit->cpu_data->mesh_id].vertices_size -
            1);
    T1_log_assert(all_mesh_vertices_tail_i < T1_mesh_summary_all_vertices->size);
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    to_commit->cpu_data->committed = true;
}

void
T1_zsprite_get_pos_xyz(
    const s32 T1_id,
    f32 * recip_x,
    f32 * recip_y,
    f32 * recip_z)
{
    f32 count = 0.0f;
    *recip_x = 0.0f;
    *recip_y = 0.0f;
    *recip_z = 0.0f;
    
    for (
        s32 zs_i = 0;
        zs_i < (s32)T1_zsprite_list->size;
        zs_i++)
    {
        if (
            T1_zsprite_list->cpu[zs_i].
                T1_id == T1_id)
        {
            count += 1.0f;
            
            *recip_x +=
                T1_zsprite_list->cpu[zs_i].
                    simd_stats.xyz[0];
            *recip_y +=
                T1_zsprite_list->cpu[zs_i].
                    simd_stats.xyz[1];
            *recip_z +=
                T1_zsprite_list->cpu[zs_i].
                    simd_stats.xyz[2];
        }
    }
    
    if (count > 0.05f) {
        *recip_x /= count;
        *recip_y /= count;
        *recip_z /= count;
    } else {
        *recip_x = 0.0f;
        *recip_y = 0.0f;
        *recip_z = 0.0f;
    }
}

void
T1_zsprite_delete(
    const s32 with_object_id)
{
    for (
        u32 i = 0;
        i < T1_zsprite_list->size;
        i++)
    {
        if (
            T1_zsprite_list->cpu[i].
                T1_id == with_object_id)
        {
            T1_zsprite_list->cpu[i].
                deleted = true;
            T1_zsprite_list->cpu[i].T1_id = -1;
        }
    }
}

void
T1_zsprite_delete_all(void) {
    T1_zsprite_list->size = 0;
}

static f32
T1_zsprite_get_x_multiplier_for_width(
    T1CPUzSprite * for_poly,
    const f32 for_width)
{
    T1_log_assert(for_poly != NULL);
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    if (for_poly == NULL) {
        return 0.0f;
    }
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_log_assert(for_poly->mesh_id >= 0);
    T1_log_assert(for_poly->mesh_id < (s32)T1_mesh_summary_list_size);
    
    T1_log_assert(for_poly->mesh_id >= 0);
    T1_log_assert(for_poly->mesh_id < (s32)T1_mesh_summary_list_size);
    
    f32 return_value =
        for_width / T1_mesh_summary_list[for_poly->mesh_id].base_width;
    
    return return_value;
}

void
T1_zsprite_scale_multipliers_to_width(
    T1CPUzSprite * cpu_data,
    T1GPUzSprite * gpu_data,
    const f32 new_height)
{
    (void)gpu_data;
    
    f32 new_multiplier = T1_zsprite_get_x_multiplier_for_width(
        /* zPolygonCPU * for_poly: */
            cpu_data,
        /* const f32 for_height: */
            new_height);
    
    cpu_data->simd_stats.mul_xyz[0] = new_multiplier;
    cpu_data->simd_stats.mul_xyz[1] = new_multiplier;
    cpu_data->simd_stats.mul_xyz[2] = new_multiplier;
}

void
T1_zsprite_scale_multipliers_to_height(
    T1CPUzSprite * cpu_data,
    T1GPUzSprite * gpu_data,
    const f32 new_height)
{
    (void)gpu_data;
    
    f32 new_multiplier = T1_global_get_y_mul_for_height(
        /* zPolygonCPU * for_poly: */
            cpu_data->mesh_id,
        /* const f32 for_height: */
            new_height);
    
    cpu_data->simd_stats.mul_xyz[0] = new_multiplier;
    cpu_data->simd_stats.mul_xyz[1] = new_multiplier;
    cpu_data->simd_stats.mul_xyz[2] = new_multiplier;
}

void
T1_zsprite_construct_with_mesh_id(
    T1zSpriteRequest * to_construct,
    const s32 mesh_id)
{
    T1_zsprite_construct(to_construct);
    
    to_construct->cpu_data->mesh_id = mesh_id;
    
    if (
        mesh_id >= 0 &&
        T1_mesh_summary_list[mesh_id].
            locked_material_base_offset != UINT32_MAX)
    {
        u32 base_mat_i = T1_mesh_summary_list[mesh_id].
            locked_material_head_i +
                T1_mesh_summary_list[mesh_id].
                    locked_material_base_offset;
        
        to_construct->gpu_data->f32s.base_mat_f32 =
            all_mesh_materials->gpu_f32[base_mat_i];
        to_construct->gpu_data->s32.base_mat_s32 =
            all_mesh_materials->gpu_s32[base_mat_i];
        
        T1_material_construct(
            &to_construct->gpu_data->f32s.base_mat_f32,
            &to_construct->gpu_data->s32.base_mat_s32);
    }
}

void T1_zsprite_construct(
    T1zSpriteRequest * to_construct)
{
    assert(to_construct->cpu_data != NULL);
    assert(to_construct->gpu_data != NULL);
    
    T1_std_memset(
        to_construct->cpu_data,
        0,
        sizeof(T1CPUzSprite));
    T1_std_memset(
        to_construct->gpu_data,
        0,
        sizeof(T1GPUzSprite));
    
    to_construct->cpu_data->simd_stats.mul_xyz[0] = 1.0f;
    to_construct->cpu_data->simd_stats.mul_xyz[1] = 1.0f;
    to_construct->cpu_data->simd_stats.mul_xyz[2] = 1.0f;
    to_construct->gpu_data->s32.touch_id = -1;
    to_construct->gpu_data->f32s.alpha = 1.0f;
    
    to_construct->cpu_data->simd_stats.scale_factor = 1.0f;
    to_construct->cpu_data->mesh_id = -1;
    to_construct->cpu_data->T1_id = -1;
    to_construct->cpu_data->visible = true;
    
    #if T1_OUTLINES_ACTIVE == T1_ACTIVE
    to_construct->gpu_data->f32s.outline_alpha = -1.0f;
    #elif T1_OUTLINES_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    to_construct->gpu_data->s32.mix_rv_and_mix_tex = -1;
    
    T1_material_construct(
        &to_construct->gpu_data->f32s.base_mat_f32,
        &to_construct->gpu_data->s32.base_mat_s32);
}

void T1_zsprite_construct_quad(
    const f32 left_x,
    const f32 bottom_y,
    const f32 z,
    const f32 width,
    const f32 height,
    T1zSpriteRequest * stack_recipient)
{
    T1_log_assert(z > 0.0f);
    
    T1_zsprite_construct(stack_recipient);
    
    const f32 mid_x = left_x + (width  / 2);
    const f32 mid_y = bottom_y + (height / 2);
    
    stack_recipient->cpu_data->simd_stats.xyz[0] = mid_x;
    stack_recipient->cpu_data->simd_stats.xyz[1] = mid_y;
    stack_recipient->cpu_data->simd_stats.xyz[2] = z;
    stack_recipient->cpu_data->visible = true;
    stack_recipient->gpu_data->f32s.no_camera = false;
    
    // a quad is hardcoded in objmodel.c's init_all_meshes()
    stack_recipient->cpu_data->mesh_id = 0;
    
    // the hardcoded quad offsets range from -1.0f to 1.0f,
    // so the current width is 2.0f
    f32 current_width = 2.0f;
    f32 current_height = 2.0f;
    stack_recipient->cpu_data->simd_stats.mul_xyz[0] =
        width / current_width;
    stack_recipient->cpu_data->simd_stats.mul_xyz[1] =
        height / current_height;
    stack_recipient->cpu_data->simd_stats.mul_xyz[2] = 1.0f;
}

void T1_zsprite_construct_quad_around(
    const f32 mid_x,
    const f32 mid_y,
    const f32 z,
    const f32 width,
    const f32 height,
    T1zSpriteRequest * stack_recipient)
{
    // T1_log_assert(z > 0.0f);
    
    T1_zsprite_construct(stack_recipient);
    
    stack_recipient->cpu_data->simd_stats.xyz[0]  = mid_x;
    stack_recipient->cpu_data->simd_stats.xyz[1]  = mid_y;
    stack_recipient->cpu_data->simd_stats.xyz[2]  = z;
    stack_recipient->cpu_data->visible = true;
    
    // the hardcoded quad offsets range from -1.0f to 1.0f,
    // so the current width is 2.0f
    f32 current_width = 2.0f;
    f32 current_height = 2.0f;
    stack_recipient->cpu_data->simd_stats.mul_xyz[0] =
        width / current_width;
    stack_recipient->cpu_data->simd_stats.mul_xyz[1] =
        height / current_height;
    stack_recipient->cpu_data->simd_stats.mul_xyz[2] =
        stack_recipient->cpu_data->simd_stats.mul_xyz[1] / 20.0f;
    
    #define THRESH 0.00001f
    if (
        stack_recipient->cpu_data->simd_stats.
            mul_xyz[0] < THRESH)
    {
        stack_recipient->cpu_data->simd_stats.mul_xyz[0] = THRESH;
    }
    if (
        stack_recipient->cpu_data->simd_stats.
            mul_xyz[1] < THRESH)
    {
        stack_recipient->cpu_data->simd_stats.mul_xyz[1] = THRESH;
    }
    if (stack_recipient->cpu_data->simd_stats.
        mul_xyz[2] < THRESH)
    {
        stack_recipient->cpu_data->simd_stats.mul_xyz[2] = THRESH;
    }
    
    stack_recipient->cpu_data->mesh_id =
        T1_BASIC_QUAD_MESH_ID;
}

void
T1_zsprite_construct_cube_around(
    const f32 mid_x,
    const f32 mid_y,
    const f32 z,
    const f32 width,
    const f32 height,
    const f32 depth,
    T1zSpriteRequest * stack_recipient)
{
    T1_log_assert(z > 0.0f);
    
    T1_zsprite_construct(stack_recipient);
    
    stack_recipient->cpu_data->simd_stats.xyz[0]  = mid_x;
    stack_recipient->cpu_data->simd_stats.xyz[1]  = mid_y;
    stack_recipient->cpu_data->simd_stats.xyz[2]  = z;
    stack_recipient->cpu_data->visible = true;
    
    // the hardcoded quad offsets range from -1.0f to 1.0f,
    // so the current width is 2.0f
    f32 current_width = 2.0f;
    f32 current_height = 2.0f;
    f32 current_depth = 2.0f;
    stack_recipient->cpu_data->simd_stats.mul_xyz[0] = width / current_width;
    stack_recipient->cpu_data->simd_stats.mul_xyz[1] = height / current_height;
    stack_recipient->cpu_data->simd_stats.mul_xyz[2] = depth / current_depth;
    
    stack_recipient->cpu_data->mesh_id = 1;
}

void T1_zsprite_anim_apply_effects_at_t(
    const f32 t_applied,
    const f32 t_now,
    const f32 * anim_gpu_f32s,
    const s32 * anim_gpu_s32s,
    const f32 * anim_cpu_vals,
    T1GPUzSprite * recip_gpu,
    T1CPUzSpriteSimdStats * recip_cpu)
{
    SIMD_FLOAT simd_t_now = simd_set1_f32(t_now);
    SIMD_FLOAT simd_t_b4  = simd_set1_f32(t_applied);
    
    T1_log_assert((sizeof(T1GPUzSpritef32) / 4) %
        SIMD_FLOAT_LANES == 0);
    T1_log_assert((sizeof(T1GPUzSprites32) / 4) %
        SIMD_INT32_LANES == 0);
    T1_log_assert((sizeof(T1CPUzSpriteSimdStats) / 4) %
        SIMD_INT32_LANES == 0);
    
    assert_sanity_check_zsprite_vals(
        recip_gpu,
        recip_cpu);
    
    if (anim_gpu_f32s) {
        f32 * target_vals_ptr =
            (f32 *)(&recip_gpu->f32s);
        
        for (
            u32 simd_step_i = 0;
            (simd_step_i * sizeof(f32)) < sizeof(T1GPUzSpritef32);
            simd_step_i += SIMD_FLOAT_LANES)
        {
            SIMD_FLOAT simd_anim_vals =
                simd_load_f32s(
                    (anim_gpu_f32s + simd_step_i));
            SIMD_FLOAT simd_target_vals =
                simd_load_f32s(
                    (target_vals_ptr + simd_step_i));
            
            SIMD_FLOAT simd_t_now_deltas =
                simd_mul_f32s(
                    simd_anim_vals,
                    simd_t_now);
            SIMD_FLOAT simd_t_previous_deltas =
                simd_mul_f32s(
                    simd_anim_vals,
                    simd_t_b4);
            
            simd_t_now_deltas = simd_sub_f32s(
                simd_t_now_deltas,
                simd_t_previous_deltas);
            
            simd_store_f32s(
                (target_vals_ptr + simd_step_i),
                simd_add_f32s(
                    simd_target_vals,
                    simd_t_now_deltas));
        }
        
        assert_sanity_check_zsprite_vals(
            recip_gpu,
            recip_cpu);
    }
    
    if (anim_gpu_s32s) {
        T1_log_assert(t_now == 1.0f);
        
        s32 * target_s32s_ptr = (s32 *)
            (&recip_gpu->s32);
        
        // Int32 values
        for (
            u32 simd_step_i = 0;
            (simd_step_i * sizeof(s32)) < sizeof(T1GPUzSprites32);
            simd_step_i += SIMD_INT32_LANES)
        {
            SIMD_INT32 simd_add_s32s =
                simd_load_int32s(
                    (anim_gpu_s32s + simd_step_i));
            
            SIMD_INT32 simd_cur_s32s =
                simd_load_int32s(
                    (target_s32s_ptr +
                        simd_step_i));
                        
            SIMD_INT32 result = simd_add_int32s(
                simd_cur_s32s,
                simd_add_s32s);
            
            simd_store_int32s(
                target_s32s_ptr + simd_step_i,
                result);
        }
        
        assert_sanity_check_zsprite_vals(
            recip_gpu,
            recip_cpu);
    }
    
    if (!anim_cpu_vals) { return; }
    
    f32 * target_vals_ptr = (f32 *)recip_cpu;
    
    for (
        u32 simd_step_i = 0;
        (simd_step_i * sizeof(f32)) <
            sizeof(T1CPUzSpriteSimdStats);
        simd_step_i += SIMD_FLOAT_LANES)
    {
        SIMD_FLOAT simd_anim_vals =
            simd_load_f32s((anim_cpu_vals + simd_step_i));
        SIMD_FLOAT simd_target_vals =
            simd_load_f32s((target_vals_ptr + simd_step_i));
        
        SIMD_FLOAT simd_t_now_deltas =
            simd_mul_f32s(
                simd_anim_vals,
                simd_t_now);
        SIMD_FLOAT simd_t_previous_deltas =
            simd_mul_f32s(
                simd_anim_vals,
                simd_t_b4);
        
        simd_t_now_deltas = simd_sub_f32s(
            simd_t_now_deltas,
            simd_t_previous_deltas);
        
        simd_store_f32s(
            (target_vals_ptr + simd_step_i),
            simd_add_f32s(
                simd_target_vals,
                simd_t_now_deltas));
    }
    
    assert_sanity_check_zsprite_vals(
        recip_gpu,
        recip_cpu);
}

void T1_zsprite_apply_anim_effects_to_id(
    const s32 T1_id,
    const s32 touch_id,
    const f32 t_applied,
    const f32 t_now,
    const f32 * anim_gpu_vals_f32,
    const s32 * anim_gpu_vals_s32,
    const f32 * anim_cpu_vals)
{
    for (
        s32 zp_i = 0;
        zp_i < (s32)T1_zsprite_list->size;
        zp_i++)
    {
        if (
            (T1_id >= 0 &&
            T1_zsprite_list->cpu[zp_i].
                T1_id != T1_id &&
            T1_id != T1_ZSPRITE_ID_HIT_EVERYTHING) ||
            (touch_id >= 0 &&
            T1_zsprite_list->gpu[zp_i].s32.
                touch_id != touch_id) ||
            T1_zsprite_list->cpu[zp_i].deleted)
        {
            continue;
        }
        
        T1_zsprite_anim_apply_effects_at_t(
            /* const f32 t_applied: */
                t_applied,
            /* const f32 t_now: */
                t_now,
            /* const f32 * anim_gpu_f32s: */
                anim_gpu_vals_f32,
            /* const s32 * anim_gpu_s32s: */
                anim_gpu_vals_s32,
            /* const f32 * anim_cpu_vals: */
                anim_cpu_vals,
            /* T1GPUzSprite * recip_gpu: */
                &T1_zsprite_list->gpu[zp_i],
            /* T1CPUzSpriteSimdStats * recip_cpu: */
                &T1_zsprite_list->cpu[zp_i].
                    simd_stats);
    }
}

void T1_zsprite_apply_endpoint_anim(
    const s32 T1_id,
    const s32 touch_id,
    const f32 t_applied,
    const f32 t_now,
    const f32 * goal_gpu_vals_f32,
    const s32 * goal_gpu_vals_s32,
    const f32 * goal_cpu_vals)
{
    // When t is 1.0f, all of our stats will
    // be exactly equal to target_delta
    const f32 was_left_t = 1.0f - t_applied;
    const f32 did_now_t = t_now - t_applied;
    const f32 t_mult = did_now_t / was_left_t;
    SIMD_FLOAT simd_t = simd_set1_f32(t_mult);
    
    f32 no_effect = T1_ZSPRITEANIM_NO_EFFECT;
    SIMD_FLOAT simd_noeffect =
        simd_set1_f32(no_effect);
    
    for (
        s32 zp_i = 0;
        zp_i < (s32)T1_zsprite_list->size;
        zp_i++)
    {
        if (
            T1_id !=
                T1_ZSPRITE_ID_HIT_EVERYTHING &&
            ((T1_id >= 0 &&
            T1_zsprite_list->cpu[zp_i].
                T1_id != T1_id) ||
            (touch_id >= 0 &&
            T1_zsprite_list->gpu[zp_i].s32.
                touch_id != touch_id) ||
            T1_zsprite_list->cpu[zp_i].deleted))
        {
            continue;
        }
        
        assert_sanity_check_zsprite_vals_by_id(zp_i);
        
        if (goal_cpu_vals) {
            f32 * recip_vals_cpu = (f32 *)
            &T1_zsprite_list->cpu[zp_i].
                simd_stats;
            
            for (
                u32 simd_step_i = 0;
                (simd_step_i * sizeof(f32)) <
                    sizeof(T1CPUzSpriteSimdStats);
                simd_step_i += SIMD_FLOAT_LANES)
            {
                SIMD_FLOAT simd_goal_vals =
                    simd_load_f32s(
                        goal_cpu_vals +
                            simd_step_i);
                
                SIMD_FLOAT simd_cur_vals =
                    simd_load_f32s((recip_vals_cpu +
                        simd_step_i));
                
                SIMD_FLOAT delta_to_goal =
                    simd_sub_f32s(
                        simd_goal_vals,
                        simd_cur_vals);
                
                delta_to_goal = simd_mul_f32s(delta_to_goal, simd_t);
                
                SIMD_FLOAT flags = simd_not_f32s(
                    simd_cmpeq_f32s(
                        simd_goal_vals,
                        simd_noeffect));
                
                delta_to_goal = simd_and_f32s(
                    delta_to_goal, flags);
                
                simd_cur_vals = simd_add_f32s(
                    simd_cur_vals,
                    delta_to_goal);
                
                simd_store_f32s(
                    recip_vals_cpu + simd_step_i,
                    simd_cur_vals);
            }
        }
        
        if (goal_gpu_vals_f32) {
            f32 * recip_vals_gpu = (f32 *)
                &T1_zsprite_list->gpu[zp_i].
                    f32s;
            
            for (
                u32 simd_step_i = 0;
                (simd_step_i * sizeof(f32)) < sizeof(T1GPUzSpritef32);
                simd_step_i += SIMD_FLOAT_LANES)
            {
                SIMD_FLOAT simd_goal_vals =
                    simd_load_f32s(
                        (goal_gpu_vals_f32 +
                            simd_step_i));
                
                SIMD_FLOAT simd_cur_vals =
                    simd_load_f32s(
                        (recip_vals_gpu +
                            simd_step_i));
                
                SIMD_FLOAT delta_to_goal =
                    simd_sub_f32s(
                        simd_goal_vals,
                            simd_cur_vals);
                
                delta_to_goal = simd_mul_f32s(
                    delta_to_goal,
                    simd_t);
                
                SIMD_FLOAT flags = simd_not_f32s(
                    simd_cmpeq_f32s(
                        simd_goal_vals,
                        simd_noeffect));
                
                delta_to_goal = simd_and_f32s(
                    delta_to_goal, flags);
                
                simd_cur_vals = simd_add_f32s(
                    simd_cur_vals,
                    delta_to_goal);
                
                simd_store_f32s(
                    recip_vals_gpu + simd_step_i,
                    simd_cur_vals);
            }
            
            assert_sanity_check_zsprite_vals_by_id(zp_i);
        }
        
        if (goal_gpu_vals_s32) {
            s32 zero_s32 = 0;
            SIMD_INT32 simd_all_zeros =
                simd_set1_int32s(zero_s32);
            
            s32 * recip_vals_s32 = (s32 *)
                &T1_zsprite_list->gpu[zp_i].s32;
            T1_log_assert(recip_vals_s32[0] ==
                T1_zsprite_list->gpu[zp_i].s32.base_mat_s32.normalmap_tex_and_tex);
            T1_log_assert(t_applied == 0.0f);
            T1_log_assert(t_now == 1.0f);
            
            for (
                u32 simd_step_i = 0;
                (simd_step_i * sizeof(s32)) <
                    sizeof(T1GPUzSprites32);
                simd_step_i += SIMD_FLOAT_LANES)
            {
                SIMD_INT32 simd_goal_s32s =
                    simd_load_int32s(
                        (goal_gpu_vals_s32 +
                            simd_step_i));
                
                SIMD_INT32 simd_cur_s32s =
                    simd_load_int32s(
                        (recip_vals_s32 + simd_step_i));
                
                SIMD_FLOAT simd_flags_f32 =
                    simd_load_f32s(
                        ((f32 *)goal_gpu_vals_s32 + simd_step_i));
                
                simd_flags_f32 = simd_cmpeq_f32s(
                    simd_flags_f32,
                    simd_noeffect);
                SIMD_INT32 simd_flags_s32;
                T1_std_memcpy(
                    &simd_flags_s32,
                    &simd_flags_f32,
                    sizeof(SIMD_INT32));
                
                s32 t_now_s32 = (s32)t_now;
                SIMD_INT32 simd_t_now_s32 = simd_set1_int32s(t_now_s32);
                
                simd_t_now_s32 = simd_cmpgt_int32s(
                        simd_t_now_s32,
                        simd_all_zeros);
                
                simd_flags_s32 = simd_and_int32s(
                    simd_flags_s32,
                    simd_t_now_s32);
                
                SIMD_INT32 results_s32 = simd_add_int32s(
                        simd_and_int32s(
                            simd_goal_s32s,
                            simd_not_int32s(
                                simd_flags_s32)),
                        simd_and_int32s(
                            simd_cur_s32s,
                            simd_flags_s32));
                
                simd_store_int32s(
                    recip_vals_s32 + simd_step_i,
                    results_s32);
            }
            
            assert_sanity_check_zsprite_vals_by_id(zp_i);
        }
    }
}

#if T1_OCCLUSION_ACTIVE == T1_ACTIVE
void T1_zsprite_set_occlusion(
    const s32 T1_id,
    const s32 new_visible_stat,
    const u64 wait_before_invis_us)
{
    for (
        s32 zp_i = 0;
        zp_i < (s32)T1_zsprite_list->size;
        zp_i++)
    {
        if (
            T1_zsprite_list->cpu[zp_i].T1_id != T1_id)
        {
            continue;
        }
        
        if (!new_visible_stat)
        {
            if (wait_before_invis_us > 0)
            {
                if (
                    T1_zsprite_list->cpu[zp_i].
                        visible)
                {
                    T1_zsprite_list->cpu[zp_i].
                        next_occlusion_in_us =
                            wait_before_invis_us;
                } else {
                    T1_zsprite_list->cpu[zp_i].
                        next_occlusion_in_us = 0;
                }
            } else {
                T1_zsprite_list->cpu[zp_i].
                    next_occlusion_in_us = 0;
                T1_zsprite_list->cpu[zp_i].
                    visible = 0;
            }
        } else {
            T1_zsprite_list->cpu[zp_i].
                next_occlusion_in_us = 0;
            T1_zsprite_list->cpu[zp_i].
                visible = 1;
        }
    }
}
#elif T1_OCCLUSION_ACTIVE == T1_INACTIVE
#else
#error
#endif

void T1_zsprite_handle_timed_occlusion(void)
{
    for (
        u32 zs_i = 0;
        zs_i < T1_zsprite_list->size;
        zs_i++)
    {
        if (
            T1_zsprite_list->cpu[zs_i].
                next_occlusion_in_us >
                    T1_global->elapsed)
        {
            T1_zsprite_list->cpu[zs_i].
                next_occlusion_in_us -=
                    T1_global->elapsed;
        } else if (
            T1_zsprite_list->cpu[zs_i].
                next_occlusion_in_us > 0)
        {
            T1_zsprite_list->cpu[zs_i].
                next_occlusion_in_us = 0;
            T1_zsprite_list->cpu[zs_i].
                visible = 0;
        }
    }
}

void T1_zsprite_anim_set_ignore_camera_but_retain_screenspace_pos(
    const s32 T1_id,
    const f32 new_ignore_camera)
{
    T1GPUzSprite * zs = NULL;
    T1CPUzSprite * zs_cpu = NULL;
    
    for (u32 i = 0; i < T1_zsprite_list->size; i++)
    {
        if (T1_zsprite_list->cpu[i].T1_id == T1_id) {
            zs = T1_zsprite_list->gpu + i;
            zs_cpu = T1_zsprite_list->cpu + i;
        }
    }
    
    if (zs->f32s.no_camera == new_ignore_camera)
    {
        return;
    }
    
    // For now we're only supporting the easy case of a full toggle
    u8 is_near_zero =
        zs->f32s.no_camera > -0.01f &&
        zs->f32s.no_camera <  0.01f;
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    u8 is_near_one =
        zs->f32s.no_camera >  0.99f &&
        zs->f32s.no_camera <  1.01f;
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    T1_log_assert(is_near_zero || is_near_one);
    
    if (is_near_zero) {
        T1_log_assert(new_ignore_camera == 1.0f);
        
        zs_cpu->simd_stats.xyz[0] -= T1_cam->xyz[0];
        zs_cpu->simd_stats.xyz[1] -= T1_cam->xyz[1];
        zs_cpu->simd_stats.xyz[2] -= T1_cam->xyz[2];
        T1_triangle_x_rotate_f3(zs_cpu->simd_stats.xyz, -T1_cam->angle_xyz[0]);
        T1_triangle_y_rotate_f3(zs_cpu->simd_stats.xyz, -T1_cam->angle_xyz[1]);
        T1_triangle_z_rotate_f3(zs_cpu->simd_stats.xyz, -T1_cam->angle_xyz[2]);
        
        #if 1
        // This is a hack, an approximation
        zs_cpu->simd_stats.angle_xyz[0] -= T1_cam->angle_xyz[0];
        zs_cpu->simd_stats.angle_xyz[1] -= T1_cam->angle_xyz[1];
        zs_cpu->simd_stats.angle_xyz[2] -= T1_cam->angle_xyz[2];
        #endif
        
        zs->f32s.no_camera = 1.0f;
    } else {
        T1_log_assert(is_near_one);
        
        T1_triangle_z_rotate_f3(zs_cpu->simd_stats.xyz, T1_cam->angle_xyz[2]);
        T1_triangle_y_rotate_f3(zs_cpu->simd_stats.xyz, T1_cam->angle_xyz[1]);
        T1_triangle_x_rotate_f3(zs_cpu->simd_stats.xyz, T1_cam->angle_xyz[0]);
        
        zs_cpu->simd_stats.xyz[0] += T1_cam->xyz[0];
        zs_cpu->simd_stats.xyz[1] += T1_cam->xyz[1];
        zs_cpu->simd_stats.xyz[2] += T1_cam->xyz[2];
        
        #if 1
        // This is a hack, an approximation
        zs_cpu->simd_stats.angle_xyz[0] += T1_cam->angle_xyz[0];
        zs_cpu->simd_stats.angle_xyz[1] += T1_cam->angle_xyz[1];
        zs_cpu->simd_stats.angle_xyz[2] += T1_cam->angle_xyz[2];
        #endif
        
        zs->f32s.no_camera = 0.0f;
    }
}

void T1_zsprite_copy_to_frame_data(
    T1GPUzSprite * recip,
    IdPair * recip_ids,
    u32 * recip_size)
{
    T1_log_assert(*recip_size == 0);
    
    T1_log_assert(T1_zsprite_list->size < T1_ZSPRITES_CAP);
    
    T1_std_memcpy(
        /* void * dest: */
            recip,
        /* const void * src: */
            T1_zsprite_list->gpu,
        /* u64 n: */
            sizeof(T1GPUzSprite) *
                T1_zsprite_list->size);
    
    for (
        u32 i = 0;
        i < T1_zsprite_list->size;
        i++)
    {
        recip_ids[i].T1_id = T1_zsprite_list->cpu[i].T1_id;
        recip_ids[i].touch_id = T1_zsprite_list->gpu[i].s32.
                touch_id;
    }
    
    *recip_size = T1_zsprite_list->size;
}

void T1_zsprite_add_alphablending_zpolygons_to_workload(
    T1GPUFrame * frame_data)
{
    s32 first_alpha_i =
        (s32)frame_data->verts_size;
    
    // Copy all vertices that do use alpha blending
    for (
        s32 cpu_zp_i = 0;
        cpu_zp_i < (s32)T1_zsprite_list->size;
        cpu_zp_i++)
    {
        if (
            T1_zsprite_list->cpu[cpu_zp_i].
                deleted ||
            !T1_zsprite_list->cpu[cpu_zp_i].
                visible ||
            !T1_zsprite_list->cpu[cpu_zp_i].committed ||
            T1_zsprite_list->cpu[cpu_zp_i].
                simd_stats.alpha_blending_on < 0.5f ||
            T1_zsprite_list->cpu[cpu_zp_i].
                simd_stats.bloom_on > 0.5f)
        {
            continue;
        }
        
        s32 mesh_id = T1_zsprite_list->cpu[cpu_zp_i].mesh_id;
        T1_log_assert(mesh_id >= 0);
        T1_log_assert(mesh_id < (s32)T1_mesh_summary_list_size);
        
        s32 vert_tail_i =
            T1_mesh_summary_list[mesh_id].vertices_head_i +
                T1_mesh_summary_list[mesh_id].vertices_size;
        assert(vert_tail_i < MAX_VERTICES_PER_BUFFER);
        
        for (
            s32 vert_i = T1_mesh_summary_list[mesh_id].vertices_head_i;
            vert_i < vert_tail_i;
            vert_i += 1)
        {
            frame_data->verts[frame_data->verts_size].
                locked_vertex_i = vert_i;
            frame_data->verts[frame_data->verts_size].
                polygon_i = cpu_zp_i;
            frame_data->verts_size += 1;
            T1_log_assert(
                frame_data->verts_size <
                    MAX_VERTICES_PER_BUFFER);
        }
    }
    
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
                        T1RENDERPASS_ALPHA_BLEND)
            {
                T1_render_views->cpu[cam_i].
                    passes[pass_i].vert_i =
                        first_alpha_i;
                T1_render_views->cpu[cam_i].
                    passes[pass_i].verts_size =
                        (s32)frame_data->
                            verts_size - first_alpha_i;
            }
        }
    }
}

void T1_zsprite_add_bloom_zpolygons_to_workload(
    T1GPUFrame * frame_data)
{
    s32 first_bloom_i = (s32)frame_data->
        verts_size;
    
    // Copy all vertices that do use bloom
    for (
        s32 cpu_zp_i = 0;
        cpu_zp_i < (s32)T1_zsprite_list->size;
        cpu_zp_i++)
    {
        if (
            T1_zsprite_list->cpu[cpu_zp_i].
                deleted ||
            !T1_zsprite_list->cpu[cpu_zp_i].
                visible ||
            !T1_zsprite_list->cpu[cpu_zp_i].
                committed ||
            T1_zsprite_list->cpu[cpu_zp_i].
                simd_stats.bloom_on < 0.5f)
        {
            continue;
        }
        
        T1_log_assert(
            T1_zsprite_list->cpu[cpu_zp_i].
                simd_stats.alpha_blending_on < 0.5f);
        T1_log_assert(
            T1_zsprite_list->cpu[cpu_zp_i].
                simd_stats.bloom_on > 0.5f);
        
        s32 mesh_id =
            T1_zsprite_list->cpu[cpu_zp_i].
                mesh_id;
        T1_log_assert(mesh_id >= 0);
        T1_log_assert(mesh_id < (s32)T1_mesh_summary_list_size);
        
        s32 vert_tail_i =
            T1_mesh_summary_list[mesh_id].
                vertices_head_i +
                    T1_mesh_summary_list[mesh_id].
                        vertices_size;
        assert(vert_tail_i < MAX_VERTICES_PER_BUFFER);
        
        for (
            s32 vert_i =
                T1_mesh_summary_list[mesh_id].
                    vertices_head_i;
            vert_i < vert_tail_i;
            vert_i += 1)
        {
            frame_data->verts[frame_data->verts_size].
                locked_vertex_i = vert_i;
            frame_data->verts[frame_data->verts_size].
                polygon_i = cpu_zp_i;
            frame_data->verts_size += 1;
            T1_log_assert(
                frame_data->verts_size <
                    MAX_VERTICES_PER_BUFFER);
        }
    }
    
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
                        T1RENDERPASS_BLOOM)
            {
                T1_render_views->cpu[cam_i].
                    passes[pass_i].vert_i =
                        first_bloom_i;
                T1_log_assert(frame_data->
                    verts_size < INT32_MAX);
                T1_render_views->cpu[cam_i].
                    passes[pass_i].verts_size =
                        (s32)frame_data->
                            verts_size -
                                first_bloom_i;
            }
        }
    }
}


void T1_zsprite_add_opaque_zpolygons_to_workload(
    T1GPUFrame * frame_data)
{
    // for now we assume this always comes 1st
    T1_log_assert(frame_data->verts_size == 0);
    
    s32 first_opaq_i = (s32)frame_data->verts_size;
    
    s32 cur_vals[4];
    s32 incr_vals[4];
    incr_vals[0] = 2;
    incr_vals[1] = 0;
    incr_vals[2] = 2;
    incr_vals[3] = 0;
    SIMD_VEC4I incr = simd_load_vec4i(incr_vals);
    
    T1_log_assert(T1_zsprite_list->size < T1_ZSPRITES_CAP);
    for (
        s32 cpu_zp_i = 0;
        cpu_zp_i < (s32)T1_zsprite_list->size;
        cpu_zp_i++)
    {
        if (
            T1_zsprite_list->cpu[cpu_zp_i].deleted ||
            !T1_zsprite_list->cpu[cpu_zp_i].visible ||
            !T1_zsprite_list->cpu[cpu_zp_i].committed ||
            T1_zsprite_list->cpu[cpu_zp_i].simd_stats.
                alpha_blending_on > 0.5f ||
            T1_zsprite_list->cpu[cpu_zp_i].
                simd_stats.bloom_on > 0.5f)
        {
            continue;
        }
        
        s32 mesh_id = T1_zsprite_list->cpu[cpu_zp_i].mesh_id;
        T1_log_assert(mesh_id >= 0);
        T1_log_assert(mesh_id < (s32)T1_mesh_summary_list_size);
        
        s32 vert_tail_i =
            T1_mesh_summary_list[mesh_id].vertices_head_i + 
            T1_mesh_summary_list[mesh_id].vertices_size;
        T1_log_assert(vert_tail_i < MAX_VERTICES_PER_BUFFER);
        
        /*
        We are free to overflow the vertices buffer, since its end is not
        in use yet anyway.
        */
        s32 vert_i = T1_mesh_summary_list[mesh_id].
            vertices_head_i;
        cur_vals[0] = vert_i-2;
        cur_vals[1] = cpu_zp_i;
        cur_vals[2] = vert_i-1;
        cur_vals[3] = cpu_zp_i;
        SIMD_VEC4I cur = simd_load_vec4i(cur_vals);
        
        s32 verts_to_copy = vert_tail_i - vert_i;
        
        for (
            s32 i = 0;
            i < verts_to_copy;
            i += 2)
        {
            cur = simd_add_vec4i(cur, incr);
            simd_store_vec4i(
                (frame_data->verts + frame_data->verts_size),
                cur);
            frame_data->verts_size += 2;
        }
        
        if (verts_to_copy % 2 == 1) {
            frame_data->verts_size -= 1;
        }
    }
    
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
                    T1RENDERPASS_DIAMOND_ALPHA)
            {
                T1_render_views->cpu[cam_i].
                    passes[pass_i].vert_i = first_opaq_i;
                T1_render_views->cpu[cam_i].passes[pass_i].
                    verts_size = (s32)frame_data->verts_size;
            }
        }
    }
}

void
T1_zsprite_construct_model_and_normal_matrices(
    T1GPUFrame * f)
{
    T1_linal_f32x4x4 result;
    T1_linal_f32x4x4 next;
    
    T1_linal_f32x3x3 model3x3;
    // T1_linal_f323x3 view3x3;
    
    for (
        u32 i = 0;
        i < T1_zsprite_list->size;
        i++)
    {
        T1CPUzSpriteSimdStats * s =
            &T1_zsprite_list->cpu[i].
                simd_stats;
        
        T1_linal_f32x4x4_construct_identity(&result);
        
        // Translation
        T1_linal_f32x4x4_construct(
            &next,
            1.0f, 0.0f, 0.0f, s->xyz[0],
            0.0f, 1.0f, 0.0f, s->xyz[1],
            0.0f, 0.0f, 1.0f, s->xyz[2],
            0.0f, 0.0f, 0.0f, 1.0f);
        
        T1_linal_f32x4x4_mul_f32x4x4_inplace(
            &result, &next);
        
        T1_linal_f32x4x4_construct_xyz_rotation(
            &next,
            s->angle_xyz[0],
            s->angle_xyz[1],
            s->angle_xyz[2]);
        
        T1_linal_f32x4x4_mul_f32x4x4_inplace(
            &result,
            &next);
        
        T1_linal_f32x4x4_construct(
            &next,
            1.0f, 0.0f, 0.0f, s->offset_xyz[0],
            0.0f, 1.0f, 0.0f, s->offset_xyz[1],
            0.0f, 0.0f, 1.0f, s->offset_xyz[2],
            0.0f, 0.0f, 0.0f, 1.0f);
        
        T1_linal_f32x4x4_mul_f32x4x4_inplace(
            &result, &next);
        
        T1_linal_f32x4x4_construct(
            &next,
            s->mul_xyz[0], 0.0f, 0.0f, 0.0f,
            0.0f, s->mul_xyz[1], 0.0f, 0.0f,
            0.0f, 0.0f, s->mul_xyz[2], 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
        T1_linal_f32x4x4_mul_f32x4x4_inplace(
            &result, &next);
        
        T1_std_memcpy(
            f->matrices[i].m_4x4 + 0,
            result.rows[0].data,
            sizeof(f32) * 4);
        T1_std_memcpy(
            f->matrices[i].m_4x4 + 4,
            result.rows[1].data,
            sizeof(f32) * 4);
        T1_std_memcpy(
            f->matrices[i].m_4x4 + 8,
            result.rows[2].data,
            sizeof(f32) * 4);
        T1_std_memcpy(
            f->matrices[i].m_4x4 + 12,
            result.rows[3].data,
            sizeof(f32) * 4);
        
        // Next: transforming normals
        // store topleft 3x3 of "model to world"
        // matrix in model3x3
        T1_linal_f32x4x4_extract_f32x3x3(
            /* const T1_linal_f324x4 * in: */
                &result,
            /* const s32 omit_row_i: */
                3,
            /* const s32 omit_col_i: */
                3,
            /* T1_linal_f323x3 * out: */
                &model3x3);
        
        // inverse transpose the topleft 3x3
        T1_linal_f32x3x3_inverse_transpose_inplace(&model3x3);
        
        // store as the "normal to world" matrix
        f->matrices[i].norm_3x3[0] = model3x3.rows[0].data[0];
        f->matrices[i].norm_3x3[1] = model3x3.rows[0].data[1];
        f->matrices[i].norm_3x3[2] = model3x3.rows[0].data[2];
        f->matrices[i].norm_3x3[3] = model3x3.rows[1].data[0];
        f->matrices[i].norm_3x3[4] = model3x3.rows[1].data[1];
        f->matrices[i].norm_3x3[5] = model3x3.rows[1].data[2];
        f->matrices[i].norm_3x3[6] = model3x3.rows[2].data[0];
        f->matrices[i].norm_3x3[7] = model3x3.rows[2].data[1];
        f->matrices[i].norm_3x3[8] = model3x3.rows[2].data[2];
    }
}
void
T1_zsprite_copy_data_for_shatter_effect(
    const s32 T1_id,
    T1GPUzSprite * gpu_recip,
    T1CPUzSprite * cpu_recip)
{
    (void)cpu_recip;
    
    b8 found = false;
    
    for (
        s32 zp_i = 0;
        zp_i < (s32)T1_zsprite_list->size;
        zp_i++)
    {
        if (T1_zsprite_list->cpu[zp_i].T1_id != T1_id)
        {
            continue;
        }
        
        if (!found) {
            found = true;
            T1_std_memcpy(
                gpu_recip,
                T1_zsprite_list->gpu + zp_i,
                sizeof(T1GPUzSprite));
            T1_std_memcpy(
                gpu_recip,
                T1_zsprite_list->cpu + zp_i,
                sizeof(T1CPUzSprite));
        }
        
        T1_zsprite_list->cpu[zp_i].deleted = true;
    }
}

