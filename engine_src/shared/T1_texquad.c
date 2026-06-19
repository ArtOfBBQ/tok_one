#include "T1_texquad.h"

#include <stdlib.h> // TODO: stop using qsort

#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
#include <math.h> // isnan check
#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif

#include "T1_mem.h"
#include "T1_simd.h"
#include "T1_tex.h"
#include "T1_log.h"


#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
static void assert_sanity_check_texquad_vals(
    T1GPUTexQuadf32 * gpu_f32,
    T1GPUTexQuads32 * gpu_s32)
{
    (void)gpu_s32;
    
    if (gpu_f32) {
        T1_log_assert(!isnan(gpu_f32->xyz[0]));
        T1_log_assert(!isnan(gpu_f32->xyz[1]));
        T1_log_assert(!isnan(gpu_f32->xyz[2]));
        T1_log_assert(!isnan(gpu_f32->rgba[0]));
        T1_log_assert(!isnan(gpu_f32->rgba[1]));
        T1_log_assert(!isnan(gpu_f32->rgba[2]));
        T1_log_assert(!isnan(gpu_f32->rgba[3]));
        T1_log_assert(!isnan(gpu_f32->wh[0]));
        T1_log_assert(!isnan(gpu_f32->wh[1]));
    }
}

#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#define assert_sanity_check_texquad_vals(x, y)
#define assert_sanity_check_texquad_vals_by_id(id)
#else
#error
#endif

typedef struct {
    T1CPUTexQuad cpu[MAX_FLATQUADS_PER_BUFFER];
    T1GPUTexQuad gpu[MAX_FLATQUADS_PER_BUFFER];
    s32 size;
} T1FlatTexQuadCollection;

static T1FlatTexQuadCollection * T1_texquads = NULL;

void T1_texquad_construct(
    T1GPUTexQuadf32 * f32s,
    T1GPUTexQuads32 * s32s)
{
    T1_std_memset(
        f32s,
        0,
        sizeof(T1GPUTexQuadf32));
    T1_std_memset(
        s32s,
        0,
        sizeof(T1GPUTexQuads32));
    
    f32s->wh[0] = 0.25f;
    f32s->wh[1] = 0.25f;
    s32s->reserved_and_tex = T1_TEX_NONE;
    s32s->touch_id = -1;
}

static void T1_texquad_construct_at_i(
    const s32 i)
{
    T1_log_assert(i >= 0);
    T1_log_assert(i  < MAX_FLATQUADS_PER_BUFFER);
    
    if (!T1_log_app_running) {
        return;
    }
    
    T1_std_memset(
        &T1_texquads->cpu[i],
        0,
        sizeof(T1CPUTexQuad));
    
    T1_texquads->cpu[i].T1_id = -1;
    T1_texquads->cpu[i].visible = 1;
    
    T1_texquad_construct(
        &T1_texquads->gpu[i].f32s,
        &T1_texquads->gpu[i].s32s);
}

void T1_texquad_init(void) {
    T1_texquads =
        T1_mem_malloc_unmanaged(
            sizeof(T1FlatTexQuadCollection));
    
    T1_std_memset(
        T1_texquads,
        0,
        sizeof(T1FlatTexQuadCollection));
}

void T1_texquad_delete(const s32 T1_id)
{
    for (
        s32 i = T1_texquads->size-1;
        i >= 0;
        i--)
    {
        if (T1_texquads->cpu[i].T1_id == T1_id)
        {
            T1_texquads->cpu[i].deleted    = true;
            T1_texquads->cpu[i].T1_id =   -1;
        }
    }
    
    while (
        T1_texquads->size > 0 &&
        T1_texquads->cpu[T1_texquads->size-1].deleted)
    {
        T1_texquads->size -= 1;
    }
}

void T1_texquad_get_avg_xyz(
    f32 * recip_xyz,
    const s32 T1_id,
    b8 * found)
{
    *found = 0;
    f32 count = 0.0f;
    recip_xyz[0] = 0.0f;
    recip_xyz[1] = 0.0f;
    recip_xyz[2] = 0.0f;
    
    for (
        s32 tq_i = 0;
        tq_i < (s32)T1_texquads->size;
        tq_i++)
    {
        if (
            T1_texquads->cpu[tq_i].T1_id ==
                T1_id)
        {
            count += 1.0f;
            recip_xyz[0] +=
                T1_texquads->gpu[tq_i].f32s.xyz[0];
            recip_xyz[1] +=
                T1_texquads->gpu[tq_i].f32s.xyz[1];
            recip_xyz[2] +=
                T1_texquads->gpu[tq_i].f32s.xyz[2];
        }
    }
    
    if (count > 0.0f) {
        recip_xyz[0] /= count;
        recip_xyz[1] /= count;
        recip_xyz[2] /= count;
    }
}

void T1_texquad_delete_all(void)
{
    T1_texquads->size = 0;
}

void T1_texquad_fetch_next(
    T1FlatTexQuadRequest * request)
{
    s32 ret_i = -1;
    
    #if 0
    for (
        s32 i = 0;
        i < (s32)T1_texquads->size;
        i++)
    {
        if (
            T1_texquads->cpu[i].deleted)
        {
            ret_i = i;
        }
    }
    #endif
    
    if (ret_i < 0) {
        ret_i = T1_texquads->size;
        T1_texquads->size += 1;
        
        T1_log_assert(T1_texquads->size > 0);
        T1_log_assert(T1_texquads->size < MAX_FLATQUADS_PER_BUFFER);
    }
    
    T1_log_assert(ret_i >= 0);
    
    T1_texquad_construct_at_i(ret_i);
    
    request->cpu = T1_texquads->cpu + ret_i;
    request->gpu = T1_texquads->gpu + ret_i;
}

void T1_texquad_defragment(void) {
    s32 i = 0;
    s32 j = (s32)T1_texquads->size-1;
    
    while (i < j) {
        if (!T1_texquads->cpu[i].deleted) {
            i++;
            continue;
        }
        
        if (T1_texquads->cpu[i].deleted) {
            T1_texquads->cpu[i] = T1_texquads->cpu[j];
            T1_texquads->gpu[i] = T1_texquads->gpu[j];
            T1_texquads->cpu[j].deleted = true;
            j--;
        }
    }
    
    while (
        T1_texquads->size > 0 &&
        T1_texquads->cpu[T1_texquads->size-1].deleted)
    {
        T1_texquads->size -= 1;
    }
}

void T1_texquad_commit(
    T1FlatTexQuadRequest * request)
{
    T1_log_assert(!request->cpu->deleted);
    T1_log_assert(request->gpu->f32s.wh[0] > 0.0f);
    T1_log_assert(request->gpu->f32s.wh[1] > 0.0f);
    
    assert_sanity_check_texquad_vals(
        /* T1GPUTexQuadf32 * gpu_f32: */
            &request->gpu->f32s,
        /* T1GPUTexQuads32 * gpu_s32: */
            &request->gpu->s32s);
    
    request->cpu->committed = 1;
}

void T1_texquad_apply_endpoint_anim(
    const s32 T1_id,
    const s32 touch_id,
    const f32 t_applied,
    const f32 t_now,
    const f32 * goal_gpu_vals_f32,
    const s32 * goal_gpu_vals_s32)
{
    // When t is 1.0f, all of our stats will
    // be exactly equal to target_delta
    const f32 was_left_t = 1.0f - t_applied;
    const f32 did_now_t = t_now - t_applied;
    const f32 t_mult = did_now_t / was_left_t;
    SIMD_FLOAT simd_t = simd_set1_f32(t_mult);
    
    f32 no_effect = T1_TEXQUADANIM_NO_EFFECT;
    SIMD_FLOAT simd_noeffect = simd_set1_f32(no_effect);
    
    for (
        s32 zp_i = 0;
        zp_i < (s32)T1_texquads->size;
        zp_i++)
    {
        if (
            T1_id !=
                T1_TEXQUAD_ID_HIT_EVERYTHING &&
            ((T1_id >= 0 &&
            T1_texquads->cpu[zp_i].
                T1_id != T1_id) ||
            (touch_id >= 0 &&
            T1_texquads->gpu[zp_i].s32s.touch_id != touch_id) ||
            T1_texquads->cpu[zp_i].deleted))
        {
            continue;
        }
        
        if (goal_gpu_vals_f32) {
            f32 * recip_vals_gpu = (f32 *)
                &T1_texquads->gpu[zp_i].f32s;
            
            for (
                u32 simd_step_i = 0;
                (simd_step_i * sizeof(f32)) < sizeof(T1GPUTexQuadf32);
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
                
                assert_sanity_check_texquad_vals(
                    &T1_texquads->gpu[zp_i].f32s,
                    &T1_texquads->gpu[zp_i].s32s);
                
                simd_store_f32s(
                    recip_vals_gpu + simd_step_i,
                    simd_cur_vals);
                
                assert_sanity_check_texquad_vals(
                    &T1_texquads->gpu[zp_i].f32s,
                    &T1_texquads->gpu[zp_i].s32s);
            }
        }
        
        if (goal_gpu_vals_s32) {
            s32 zero_s32 = 0;
            SIMD_INT32 simd_all_zeros =
                simd_set1_int32s(zero_s32);
            
            s32 * recip_vals_s32 = (s32 *)
                &T1_texquads->gpu[zp_i].s32s;
            T1_log_assert(recip_vals_s32[0] ==
                T1_texquads->gpu[zp_i].s32s.reserved_and_tex);
            
            T1_log_assert(t_applied == 0.0f);
            T1_log_assert(t_now == 1.0f);
            
            for (
                u32 simd_step_i = 0;
                (simd_step_i * sizeof(s32)) <
                    sizeof(T1GPUTexQuads32);
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
        }
    }
}

void T1_texquad_anim_apply_effects_at_t(
    const f32 t_applied,
    const f32 t_now,
    const f32 * anim_gpu_f32s,
    const s32 * anim_gpu_s32s,
    const f32 * anim_cpu_vals,
    T1GPUTexQuad * recip_gpu,
    T1CPUTexQuad * recip_cpu)
{
    (void)recip_cpu;
    
    SIMD_FLOAT simd_t_now = simd_set1_f32(t_now);
    SIMD_FLOAT simd_t_b4  = simd_set1_f32(t_applied);
    
    T1_log_assert((sizeof(T1GPUTexQuadf32) / 4) %
        SIMD_FLOAT_LANES == 0);
    T1_log_assert((sizeof(T1GPUTexQuads32) / 4) %
        SIMD_INT32_LANES == 0);
    
    if (anim_gpu_f32s) {
        f32 * target_vals_ptr = (f32 *)(&recip_gpu->f32s);
        
        for (
            u32 simd_step_i = 0;
            (simd_step_i * sizeof(f32)) < sizeof(T1GPUTexQuadf32);
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
    }
    
    if (anim_gpu_s32s) {
        T1_log_assert(t_now == 1.0f);
        
        s32 * target_s32s_ptr = (s32 *)
            (&recip_gpu->s32s);
        
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
    }
    
    if (!anim_cpu_vals) { return; }
    
    T1_log_assert(0); // no cpu vals for now
}

void T1_texquad_apply_anim_effects_to_id(
    const s32 T1_id,
    const s32 touch_id,
    const f32 t_applied,
    const f32 t_now,
    const f32 * anim_gpu_vals_f32,
    const s32 * anim_gpu_vals_s32)
{
    for (
        s32 tq_i = 0;
        tq_i < (s32)T1_texquads->size;
        tq_i++)
    {
        if (
            (T1_id >= 0 &&
            T1_texquads->cpu[tq_i].
                T1_id != T1_id &&
            T1_id != T1_TEXQUAD_ID_HIT_EVERYTHING) ||
            (touch_id >= 0 &&
            T1_texquads->gpu[tq_i].s32s.
                touch_id != touch_id) ||
            T1_texquads->cpu[tq_i].deleted)
        {
            continue;
        }
        
        T1_texquad_anim_apply_effects_at_t(
            /* const f32 t_applied: */
                t_applied,
            /* const f32 t_now: */
                t_now,
            /* const f32 * anim_gpu_f32s: */
                anim_gpu_vals_f32,
            /* const s32 * anim_gpu_s32s: */
                anim_gpu_vals_s32,
            /* : */
                NULL,
            /* : */
                &T1_texquads->gpu[tq_i],
            /* : */
                &T1_texquads->cpu[tq_i]);
    }
}

static s32 cmp_highest_z_texquad(
    const void * a,
    const void * b)
{
    f32 fa = ((T1GPUTexQuad *)a)->f32s.xyz[2];
    f32 fb = ((T1GPUTexQuad *)b)->f32s.xyz[2];
    
    if (fb < fa) {
        return -1;
    } else if (fb > fa) {
        return 1;
    } else {
        return 0;
    }
}

void T1_texquad_copy_to_frame_data(
    T1GPUTexQuad * recip_fd,
    u32 * recip_fd_size)
{
    *recip_fd_size = 0;
    for (
        s32 i = 0;
        i < T1_texquads->size;
        i++)
    {
        if (
            T1_texquads->cpu[i].visible &&
            !T1_texquads->cpu[i].deleted &&
            T1_texquads->cpu[i].committed)
        {
            recip_fd[*recip_fd_size] =
                T1_texquads->gpu[i];
            
            recip_fd[*recip_fd_size].f32s.xyz[0] +=
                T1_texquads->cpu[i].offset_xyz[0];
            recip_fd[*recip_fd_size].f32s.xyz[1] +=
                T1_texquads->cpu[i].offset_xyz[1];
            recip_fd[*recip_fd_size].f32s.xyz[2] +=
                T1_texquads->cpu[i].offset_xyz[2];
            
            *recip_fd_size += 1;
        }
        
        if (T1_texquads->cpu[i].one_frame_only) {
            T1_texquads->cpu[i].deleted = true;
        }
    }
    
    #if 1
    qsort(
        /* void *base: */
            recip_fd,
        /* u64 nel: */
            *recip_fd_size,
        /* u64 width: */
            sizeof(T1GPUTexQuad),
        /* s32 (* _Nonnull compar)(const void *, const void *): */
            cmp_highest_z_texquad);
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    for (
        u32 i = 0;
        i + 1 < *recip_fd_size;
        i++)
    {
        T1_log_assert(
            recip_fd[i].f32s.xyz[2] >= recip_fd[i+1].f32s.xyz[2]);
    }
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    #endif
}
