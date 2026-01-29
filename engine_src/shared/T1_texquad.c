#include "T1_texquad.h"

typedef struct {
    T1CPUTexQuad cpu[MAX_FLATQUADS_PER_BUFFER];
    T1GPUTexQuad gpu[MAX_FLATQUADS_PER_BUFFER];
    uint32_t size;
} T1FlatTexQuadCollection;

static T1FlatTexQuadCollection *
    T1_flat_texquads = NULL;

static void T1_flat_texquads_construct_at_i(
    const int32_t i)
{
    T1_std_memset(
        &T1_flat_texquads->cpu[i],
        0,
        sizeof(T1CPUTexQuad));
    T1_std_memset(
        &T1_flat_texquads->gpu[i],
        0,
        sizeof(T1GPUTexQuad));
    
    T1_flat_texquads->cpu[i].zsprite_id = -1;
    T1_flat_texquads->cpu[i].visible = 1;
    
    T1_flat_texquads->gpu[i].f32.size_xy[0] = 0.25f;
    T1_flat_texquads->gpu[i].f32.size_xy[1] = 0.25f;
    T1_flat_texquads->gpu[i].i32.tex_array_i = -1;
    T1_flat_texquads->gpu[i].i32.tex_slice_i = -1;
    T1_flat_texquads->gpu[i].i32.touch_id = -1;
}

void T1_texquad_init(void) {
    T1_flat_texquads =
        T1_mem_malloc_from_unmanaged(
            sizeof(T1FlatTexQuadCollection));
    
    T1_std_memset(
        T1_flat_texquads,
        0,
        sizeof(T1FlatTexQuadCollection));
}

void T1_texquad_delete(const int32_t zsprite_id)
{
    for (
        int32_t i = 0;
        i < (int32_t)T1_flat_texquads->size;
        i++)
    {
        if (
            T1_flat_texquads->cpu[i].
                zsprite_id == zsprite_id)
        {
            T1_flat_texquads->cpu[i].deleted =
                true;
            T1_flat_texquads->cpu[i].zsprite_id = -1;
        }
    }
}

void T1_texquad_delete_all(void)
{
    T1_flat_texquads->size = 0;
}

void T1_texquad_fetch_next(
    T1FlatTexQuadRequest * request)
{
    int32_t ret_i = -1;
    
    for (
        int32_t i = 0;
        i < (int32_t)T1_flat_texquads->size;
        i++)
    {
        if (
            T1_flat_texquads->cpu[i].deleted)
        {
            ret_i = i;
        }
    }
    
    if (ret_i < 0) {
        ret_i = (int32_t)T1_flat_texquads->size;
        T1_flat_texquads->size += 1;
        
        log_assert(
            T1_flat_texquads->size <
                MAX_FLATQUADS_PER_BUFFER);
    }
    
    T1_flat_texquads_construct_at_i(ret_i);
    
    request->cpu = T1_flat_texquads->cpu + ret_i;
    request->gpu = T1_flat_texquads->gpu + ret_i;
}

void T1_texquad_commit(
    T1FlatTexQuadRequest * request)
{
    log_assert(!request->cpu->deleted);
    log_assert(request->gpu->f32.size_xy[0] > 0.0f);
    log_assert(request->gpu->f32.size_xy[1] > 0.0f);
    log_assert(request->gpu->i32.tex_array_i > -2);
    log_assert(request->gpu->i32.tex_slice_i > -2);
    log_assert(
        request->gpu->i32.tex_array_i <
            TEXTUREARRAYS_SIZE);
    
    request->cpu->committed = 1;
}

void T1_texquad_apply_endpoint_anim(
    const int32_t zsprite_id,
    const int32_t touch_id,
    const float t_applied,
    const float t_now,
    const float * goal_gpu_vals_f32,
    const int32_t * goal_gpu_vals_i32,
    const float * goal_cpu_vals)
{
    // When t is 1.0f, all of our stats will
    // be exactly equal to target_delta
    const float was_left_t = 1.0f - t_applied;
    const float did_now_t = t_now - t_applied;
    const float t_mult = did_now_t / was_left_t;
    SIMD_FLOAT simd_t = simd_set1_float(t_mult);
    
    float no_effect = T1_TEXQUADANIM_NO_EFFECT;
    SIMD_FLOAT simd_noeffect =
        simd_set1_float(no_effect);
    
    for (
        int32_t zp_i = 0;
        zp_i < (int32_t)T1_flat_texquads->size;
        zp_i++)
    {
        if (
            zsprite_id !=
                T1_TEXQUAD_ID_HIT_EVERYTHING &&
            ((zsprite_id >= 0 &&
            T1_flat_texquads->cpu[zp_i].
                zsprite_id != zsprite_id) ||
            (touch_id >= 0 &&
            T1_flat_texquads->gpu[zp_i].i32.
                touch_id != touch_id) ||
            T1_flat_texquads->cpu[zp_i].deleted))
        {
            continue;
        }
        
        if (goal_cpu_vals) {
            float * recip_vals_cpu = (float *)
            &T1_flat_texquads->cpu[zp_i];
            
            for (
                uint32_t simd_step_i = 0;
                (simd_step_i * sizeof(float)) <
                    sizeof(T1CPUTexQuad);
                simd_step_i += SIMD_FLOAT_LANES)
            {
                SIMD_FLOAT simd_goal_vals =
                    simd_load_floats((goal_cpu_vals +
                        simd_step_i));
                
                SIMD_FLOAT simd_cur_vals =
                    simd_load_floats((recip_vals_cpu +
                        simd_step_i));
                
                SIMD_FLOAT delta_to_goal =
                    simd_sub_floats(
                        simd_goal_vals,
                        simd_cur_vals);
                
                delta_to_goal = simd_mul_floats(delta_to_goal, simd_t);
                
                SIMD_FLOAT flags = simd_not_floats(
                    simd_cmpeq_floats(
                        simd_goal_vals,
                        simd_noeffect));
                
                delta_to_goal = simd_and_floats(
                    delta_to_goal, flags);
                
                simd_cur_vals = simd_add_floats(
                    simd_cur_vals,
                    delta_to_goal);
                
                simd_store_floats(
                    recip_vals_cpu + simd_step_i,
                    simd_cur_vals);
            }
        }
        
        if (goal_gpu_vals_f32) {
            float * recip_vals_gpu = (float *)
                &T1_flat_texquads->gpu[zp_i].f32;
            
            for (
                uint32_t simd_step_i = 0;
                (simd_step_i * sizeof(float)) < sizeof(T1GPUzSpritef32);
                simd_step_i += SIMD_FLOAT_LANES)
            {
                SIMD_FLOAT simd_goal_vals =
                    simd_load_floats(
                        (goal_gpu_vals_f32 +
                            simd_step_i));
                
                SIMD_FLOAT simd_cur_vals =
                    simd_load_floats(
                        (recip_vals_gpu +
                            simd_step_i));
                
                SIMD_FLOAT delta_to_goal =
                    simd_sub_floats(
                        simd_goal_vals,
                            simd_cur_vals);
                
                delta_to_goal = simd_mul_floats(
                    delta_to_goal,
                    simd_t);
                
                SIMD_FLOAT flags = simd_not_floats(
                    simd_cmpeq_floats(
                        simd_goal_vals,
                        simd_noeffect));
                
                delta_to_goal = simd_and_floats(
                    delta_to_goal, flags);
                
                simd_cur_vals = simd_add_floats(
                    simd_cur_vals,
                    delta_to_goal);
                
                simd_store_floats(
                    recip_vals_gpu + simd_step_i,
                    simd_cur_vals);
            }
        }
        
        if (goal_gpu_vals_i32) {
            int32_t zero_i32 = 0;
            SIMD_INT32 simd_all_zeros =
                simd_set1_int32s(zero_i32);
            
            int32_t * recip_vals_i32 = (int32_t *)
                &T1_flat_texquads->gpu[zp_i].i32;
            log_assert(recip_vals_i32[0] ==
                T1_flat_texquads->gpu[zp_i].i32.tex_array_i);
            
            log_assert(t_applied == 0.0f);
            log_assert(t_now == 1.0f);
            
            for (
                uint32_t simd_step_i = 0;
                (simd_step_i * sizeof(int32_t)) <
                    sizeof(T1GPUTexQuadi32);
                simd_step_i += SIMD_FLOAT_LANES)
            {
                SIMD_INT32 simd_goal_i32s =
                    simd_load_int32s(
                        (goal_gpu_vals_i32 +
                            simd_step_i));
                
                SIMD_INT32 simd_cur_i32s =
                    simd_load_int32s(
                        (recip_vals_i32 + simd_step_i));
                
                SIMD_FLOAT simd_flags_f32 =
                    simd_load_floats(
                        ((float *)goal_gpu_vals_i32 + simd_step_i));
                
                simd_flags_f32 = simd_cmpeq_floats(
                    simd_flags_f32,
                    simd_noeffect);
                SIMD_INT32 simd_flags_i32;
                T1_std_memcpy(
                    &simd_flags_i32,
                    &simd_flags_f32,
                    sizeof(SIMD_INT32));
                
                int32_t t_now_i32 = (int32_t)t_now;
                SIMD_INT32 simd_t_now_i32 = simd_set1_int32s(t_now_i32);
                
                simd_t_now_i32 = simd_cmpgt_int32s(
                    simd_t_now_i32,
                    simd_all_zeros);
                
                simd_flags_i32 = simd_and_int32s(
                    simd_flags_i32,
                    simd_t_now_i32);
                
                SIMD_INT32 results_i32 = simd_add_int32s(
                        simd_and_int32s(
                            simd_goal_i32s,
                            simd_not_int32s(
                                simd_flags_i32)),
                        simd_and_int32s(
                            simd_cur_i32s,
                            simd_flags_i32));
                
                simd_store_int32s(
                    recip_vals_i32 + simd_step_i,
                    results_i32);
            }
        }
    }
}

void T1_texquad_anim_apply_effects_at_t(
    const float t_applied,
    const float t_now,
    const float * anim_gpu_vals,
    const int32_t * anim_gpu_i32s,
    const float * anim_cpu_vals,
    T1GPUTexQuad * recip_gpu,
    T1CPUTexQuad * recip_cpu)
{
    // TODO: implement me
    log_assert(0);
}

void T1_texquad_apply_anim_effects_to_id(
    const int32_t zsprite_id,
    const int32_t touch_id,
    const float t_applied,
    const float t_now,
    const float * anim_gpu_vals_f32,
    const int32_t * anim_gpu_vals_i32,
    const float * anim_cpu_vals)
{
    // TODO: implement me
    log_assert(0);
}

void T1_texquad_draw_test(
    const float width,
    const float height)
{
    T1FlatTexQuadRequest texq;
    T1_texquad_fetch_next(&texq);
    texq.gpu->f32.size_xy[0] = width;
    texq.gpu->f32.size_xy[1] = height;
    texq.gpu->f32.pos_xyz[0] = -0.75f;
    texq.gpu->f32.pos_xyz[1] = -0.75f;
    texq.gpu->f32.pos_xyz[2] = 0.05f;
    texq.gpu->i32.tex_array_i = 2;
    texq.gpu->i32.tex_slice_i = 0;
    T1_texquad_commit(&texq);
}

static int cmp_highest_z_texquad(
    const void * a,
    const void * b)
{
    float fa = ((T1GPUTexQuad *)a)->f32.pos_xyz[2];
    float fb = ((T1GPUTexQuad *)b)->f32.pos_xyz[2];
    
    if (fb < fa) {
        return -1;
    } else if (fb > fa) {
        return 1;
    } else {
        return 0;
    }
}

void T1_texquad_copy_to_frame_data(
    T1GPUTexQuad * recip_frame_data,
    uint32_t * recip_frame_data_size)
{
    *recip_frame_data_size = 0;
    for (
        uint32_t i = 0;
        i < T1_flat_texquads->size;
        i++)
    {
        if (
            T1_flat_texquads->cpu[i].visible &&
            !T1_flat_texquads->cpu[i].deleted &&
            T1_flat_texquads->cpu[i].committed)
        {
            recip_frame_data[*recip_frame_data_size] =
                T1_flat_texquads->gpu[i];
            *recip_frame_data_size += 1;
        }
    }
    
    qsort(
        /* void *base: */
            recip_frame_data,
        /* size_t nel: */
            *recip_frame_data_size,
        /* size_t width: */
            sizeof(T1GPUTexQuad),
        /* int (* _Nonnull compar)(const void *, const void *): */
            cmp_highest_z_texquad);
    
    #ifndef LOGGER_IGNORE_ASSERTS
    for (
        uint32_t i = 0;
        i + 1 < *recip_frame_data_size;
        i++)
    {
        log_assert(
            recip_frame_data[i].f32.pos_xyz[2]
                >= recip_frame_data[i+1].f32.
                    pos_xyz[2]);
    }
    #endif
}
