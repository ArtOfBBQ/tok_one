#include "T1_texquad_anim.h"

#if T1_TEXQUAD_ANIM_ACTIVE == T1_ACTIVE

typedef struct {
    T1TexQuadAnim public;
    
    /*
    If this flag is set, the target values represent endpoints, not deltas,
    until the animation is committed. During zsprite_anim_commit, the
    values will be converted to normal delta values, and the animation will
    be treated just like any other animation
    */
    uint64_t remaining_duration_us;
    uint64_t remaining_pause_us;
    float already_applied_t;
    
    bool8_t endpoints_not_deltas;
    
    bool8_t deleted;
    bool8_t committed;
} T1InternalTexQuadAnim;

#define FLT_SCHEDULEDANIM_IGNORE 0xFFFF

T1InternalTexQuadAnim * texquad_anims;
uint32_t texquad_anims_size = 0;

typedef struct {
    T1GPUTexQuad zsprite_final_pos_gpu;
    T1GPUTexQuad zsprite_deltas[2];
    uint32_t (* init_mutex_and_return_id)(void);
    void (* mutex_lock)(const uint32_t);
    void (* mutex_unlock)(const uint32_t);
    uint32_t mutex_id;
} T1TexQuadAnimState;

static T1TexQuadAnimState * tqas = NULL;

void T1_texquad_anim_init(
    uint32_t (* funcptr_init_mutex_and_return_id)(void),
    void (* funcptr_mutex_lock)(const uint32_t),
    void (* funcptr_mutex_unlock)(const uint32_t))
{
    texquad_anims = (T1InternalTexQuadAnim *)T1_mem_malloc_from_unmanaged(
        sizeof(T1InternalTexQuadAnim) * TEXQUAD_ANIMS_CAP);
    T1_std_memset(
        texquad_anims,
        0,
        sizeof(T1InternalTexQuadAnim));
    texquad_anims[0].deleted = true;
    
    tqas = (T1TexQuadAnimState *)T1_mem_malloc_from_unmanaged(
        sizeof(T1TexQuadAnimState));
    T1_std_memset(
        tqas,
        0,
        sizeof(T1TexQuadAnimState));
    
    tqas->init_mutex_and_return_id =
        funcptr_init_mutex_and_return_id;
    tqas->mutex_lock = funcptr_mutex_lock;
    tqas->mutex_unlock = funcptr_mutex_unlock;
    
    for (
        uint32_t i = 1;
        i < TEXQUAD_ANIMS_CAP;
        i++)
    {
        T1_std_memcpy(
            &texquad_anims[i],
            &texquad_anims[0],
            sizeof(T1InternalTexQuadAnim));
        log_assert(texquad_anims[i].deleted);
    }
    
    tqas->mutex_id = tqas->init_mutex_and_return_id();
}

static
T1InternalTexQuadAnim * T1_zsprite_anim_get_container(
    const T1TexQuadAnim * public_ptr)
{
    assert(
        offsetof(T1InternalTexQuadAnim, public) == 0);
    
    T1InternalTexQuadAnim * retval = (T1InternalTexQuadAnim *)public_ptr;
    
    log_assert(
        &retval->public == public_ptr);
    
    return retval;
}

static void construct_scheduled_animationA(
    T1InternalTexQuadAnim * to_construct)
{
    T1_std_memset(
        to_construct,
        0,
        sizeof(T1InternalTexQuadAnim));
    log_assert(!to_construct->committed);
    
    to_construct->public.affected_zsprite_id = -1;
    to_construct->public.affected_touchable_id = -1;
    to_construct->public.runs = 1;
    
    log_assert(!to_construct->deleted);
    log_assert(!to_construct->committed);
}

T1TexQuadAnim * T1_texquad_anim_request_next(
    bool8_t endpoints_not_deltas)
{
    tqas->mutex_lock(
        tqas->mutex_id);
    log_assert(texquad_anims_size <
        TEXQUAD_ANIMS_CAP);
    T1InternalTexQuadAnim * return_value = NULL;
    
    for (
        uint32_t i = 0;
        i < texquad_anims_size;
        i++)
    {
        if (texquad_anims[i].deleted)
        {
            return_value = &texquad_anims[i];
        }
    }
    
    if (return_value == NULL) {
        log_assert(texquad_anims_size + 1 <
            TEXQUAD_ANIMS_CAP);
        return_value =
            &texquad_anims[texquad_anims_size++];
        log_assert(return_value->deleted);
    }
    
    log_assert(return_value->deleted);
    construct_scheduled_animationA(return_value);
    
    log_assert(!return_value->committed);
    log_assert(!return_value->deleted);
    
    if (endpoints_not_deltas) {
        T1_std_memset_f32(
            &return_value->public.gpu_vals,
            FLT_SCHEDULEDANIM_IGNORE,
            sizeof(T1GPUTexQuad));
        T1_std_memset_f32(
            &return_value->public.cpu_vals,
            FLT_SCHEDULEDANIM_IGNORE,
            sizeof(T1CPUTexQuad));
        return_value->endpoints_not_deltas =
            endpoints_not_deltas;
    }
    
    tqas->mutex_unlock(tqas->mutex_id);
    
    return &return_value->public;
}

static void
apply_animation_effects_for_given_eased_t(
    T1TPair t,
    T1TexQuadAnim * anim,
    T1GPUTexQuad * recip_gpu,
    T1CPUTexQuad * recip_cpu)
{
    float * anim_vals_ptr    = (float *)&anim->gpu_vals;
    float * target_vals_ptr = (float *)recip_gpu;
    
    SIMD_FLOAT simd_t_now = simd_set1_float(t.now);
    SIMD_FLOAT simd_t_b4  = simd_set1_float(t.applied);
    
    log_assert((sizeof(T1GPUTexQuad) / 4) % SIMD_FLOAT_LANES == 0);
    log_assert((sizeof(T1CPUTexQuad) / 4) % SIMD_FLOAT_LANES == 0);
    
    for (
        uint32_t simd_step_i = 0;
        (simd_step_i * sizeof(float)) < sizeof(T1GPUTexQuad);
        simd_step_i += SIMD_FLOAT_LANES)
    {
        SIMD_FLOAT simd_anim_vals =
            simd_load_floats((anim_vals_ptr + simd_step_i));
        SIMD_FLOAT simd_target_vals =
            simd_load_floats((target_vals_ptr + simd_step_i));
        
        SIMD_FLOAT simd_t_now_deltas =
            simd_mul_floats(
                simd_anim_vals,
                simd_t_now);
        SIMD_FLOAT simd_t_previous_deltas =
            simd_mul_floats(
                simd_anim_vals,
                simd_t_b4);
        
        simd_t_now_deltas = simd_sub_floats(
            simd_t_now_deltas,
            simd_t_previous_deltas);
        
        simd_store_floats(
            (target_vals_ptr + simd_step_i),
            simd_add_floats(
                simd_target_vals,
                simd_t_now_deltas));
    }
    
    anim_vals_ptr = (float *)&anim->cpu_vals;
    target_vals_ptr = (float *)recip_cpu;
    
    for (
        uint32_t simd_step_i = 0;
        (simd_step_i * sizeof(float)) < sizeof(T1CPUTexQuad);
        simd_step_i += SIMD_FLOAT_LANES)
    {
        SIMD_FLOAT simd_anim_vals =
            simd_load_floats((anim_vals_ptr + simd_step_i));
        SIMD_FLOAT simd_target_vals =
            simd_load_floats((target_vals_ptr + simd_step_i));
        
        SIMD_FLOAT simd_t_now_deltas =
            simd_mul_floats(
                simd_anim_vals,
                simd_t_now);
        SIMD_FLOAT simd_t_previous_deltas =
            simd_mul_floats(
                simd_anim_vals,
                simd_t_b4);
        
        simd_t_now_deltas = simd_sub_floats(
            simd_t_now_deltas,
            simd_t_previous_deltas);
        
        simd_store_floats(
            (target_vals_ptr + simd_step_i),
            simd_add_floats(
                simd_target_vals,
                simd_t_now_deltas));
    }
}

static void T1_texquad_anims_get_projected_final_position_for(
    const int32_t zp_i,
    T1GPUTexQuad * recip_gpu,
    T1CPUTexQuad * recip_cpu)
{
    log_assert(zp_i >= 0);
    log_assert((uint32_t)zp_i < T1_zsprite_list->size);
    
    const int32_t zsprite_id = T1_zsprite_list->cpu_data[zp_i].zsprite_id;
    *recip_gpu = T1_zsprite_list->gpu_data[zp_i];
    *recip_cpu = T1_zsprite_list->cpu_data[zp_i].simd_stats;
    
    for (
        uint32_t sa_i = 0;
        sa_i < texquad_anims_size;
        sa_i++)
    {
        T1InternalTexQuadAnim * a =
            &texquad_anims[sa_i];
        
        if (
            a->public.affected_zsprite_id == zsprite_id &&
            !a->deleted &&
            a->committed &&
            a->public.duration_us > 0)
        {
            T1TPair t;
            t.now = 1.0f;
            t.applied = a->already_applied_t;
            
            t.now = T1_easing_t_to_eased_t(
                t.now,
                a->public.easing_type);
            t.applied = T1_easing_t_to_eased_t(
                t.applied,
                a->public.easing_type);
            
            apply_animation_effects_for_given_eased_t(
                t,
                &a->public,
                recip_gpu,
                recip_cpu);
        }
    }
}

void T1_zsprite_anim_commit(
    T1TexQuadAnim * to_commit)
{
    tqas->mutex_lock(
        tqas->mutex_id);
    
    T1InternalTexQuadAnim * parent = T1_zsprite_anim_get_container(to_commit);
    log_assert(&parent->public == to_commit);
    
    log_assert(
        to_commit->duration_us > 0);
    
    if (
        to_commit->delete_other_anims_targeting_zsprite)
    {
        for (
            uint32_t anim_i = 0;
            anim_i < texquad_anims_size;
            anim_i++)
        {
            T1InternalTexQuadAnim * a =
                &texquad_anims[anim_i];
            
            if (
                a->public.affected_zsprite_id ==
                    to_commit->affected_zsprite_id &&
                a->public.affected_touchable_id ==
                    to_commit->affected_touchable_id &&
                a->committed &&
                a->endpoints_not_deltas)
            {
                texquad_anims[anim_i].deleted = true;
            }
        }
    }
    
    if (parent->endpoints_not_deltas) {
        int32_t first_zp_i = -1;
        for (
            int32_t zp_i = 0;
            zp_i < (int32_t)T1_zsprite_list->size;
            zp_i++)
        {
            if (
                T1_zsprite_list->cpu_data[zp_i].zsprite_id ==
                    to_commit->affected_zsprite_id)
            {
                first_zp_i = zp_i;
                break;
            }
        }
        
        if (first_zp_i < 0) {
            parent->deleted = true;
            tqas->mutex_unlock(tqas->mutex_id);
            return;
        }
        log_assert(first_zp_i < (int32_t)T1_zsprite_list->size);
        
        
        T1_texquad_anims_get_projected_final_position_for(
            first_zp_i,
            &tqas->zsprite_final_pos_gpu,
            &tqas->zsprite_final_pos_cpu);
        
        float * anim_gpu_vals = (float *)&to_commit->gpu_vals;
        float * orig_gpu_vals = (float *)&tqas->zsprite_final_pos_gpu;
        
        for (
            uint32_t i = 0;
            i < (sizeof(T1GPUTexQuad) / sizeof(float));
            i++)
        {
            if (anim_gpu_vals[i] == FLT_SCHEDULEDANIM_IGNORE) {
                anim_gpu_vals[i] = 0.0f; // delta 0 is the same as 'ignore'
            } else {
                // fetch the current value
                float delta_to_target = anim_gpu_vals[i] - orig_gpu_vals[i];
                anim_gpu_vals[i] = delta_to_target;
            }
        }
        
        float * anim_cpu_vals = (float *)&to_commit->cpu_vals;
        float * orig_cpu_vals = (float *)&tqas->zsprite_final_pos_cpu;
        for (
            uint32_t i = 0;
            i < (sizeof(T1CPUzSpriteSimdStats) / sizeof(float));
            i++)
        {
            if (anim_cpu_vals[i] == FLT_SCHEDULEDANIM_IGNORE) {
                anim_cpu_vals[i] = 0.0f; // delta 0 is the same as 'ignore'
            } else {
                // fetch the current value
                float delta_to_target = anim_cpu_vals[i] - orig_cpu_vals[i];
                anim_cpu_vals[i] = delta_to_target;
            }
        }
    }
    
    log_assert(!parent->deleted);
    log_assert(!parent->committed);
    
    if (to_commit->affected_zsprite_id < 0) {
        log_assert(to_commit->affected_touchable_id >= 0);
    } else {
        log_assert(
            to_commit->affected_touchable_id == -1);
    }
    
    if (to_commit->affected_touchable_id < 0) {
        log_assert(to_commit->affected_zsprite_id >= 0);
    } else {
        log_assert(to_commit->affected_zsprite_id == -1);
    }
    
    log_assert(parent->already_applied_t == 0.0f);
    
    parent->remaining_pause_us =
        to_commit->pause_us;
    parent->remaining_duration_us =
        to_commit->duration_us;
    
    log_assert(parent->remaining_duration_us > 0);
    parent->committed = true;
    
    log_assert(parent->committed);
    log_assert(!parent->deleted);
    tqas->mutex_unlock(tqas->mutex_id);
}

void T1_zsprite_anim_evaporate_and_destroy(
    const int32_t zsprite_id,
    const uint64_t duration_us)
{
    assert(0); // TODO: reimplement
    
    #if 0
    #if T1_LOGGER_ASSERTS_ACTIVE
    #else
    (void)duration_us;
    #endif
    
    log_assert(duration_us > 0);
    log_assert(object_id >= 0);
    
    for (
        uint32_t zp_i = 0;
        zp_i < T1_zsprites_to_render->size;
        zp_i++)
    {
        if (
            T1_zsprites_to_render->cpu_data[zp_i].deleted ||
            !T1_zsprites_to_render->cpu_data[zp_i].committed ||
            T1_zsprites_to_render->cpu_data[zp_i].zsprite_id != object_id)
        {
            continue;
        }
        
        #if T1_PARTICLES_ACTIVE == T1_ACTIVE
        float duration_mod = (20000000.0f / (float)duration_us);
        
        T1ParticleEffect * vaporize_effect = T1_particle_get_next();
        vaporize_effect->zpolygon_cpu = T1_zsprites_to_render->cpu_data[zp_i];
        vaporize_effect->zpolygon_gpu = T1_zsprites_to_render->gpu_data[zp_i];
        
        uint64_t shattered_verts_size =
            (uint64_t)all_mesh_summaries[vaporize_effect->zpolygon_cpu.mesh_id].
                shattered_vertices_size;
        vaporize_effect->spawns_per_loop = (uint32_t)(
            (shattered_verts_size * 1000000) /
                (uint64_t)(duration_us + 1));
        vaporize_effect->pause_per_spawn = 10;
        vaporize_effect->verts_per_particle = 3;
        vaporize_effect->spawn_lifespan = duration_us;
        vaporize_effect->shattered = true;

        #if 0
        float xy_dist   =  0.0065f;
        float z_dist    = -0.0130f;
        float xyz_angle =  0.0100f;
        float rgb_delta =  0.00005f;
        
        vaporize_effect->pertime_rand_add[0].xyz[0] = -xy_dist *
            duration_mod;
        vaporize_effect->pertime_rand_add[0].xyz[1] = -xy_dist *
            duration_mod;
        vaporize_effect->pertime_rand_add[0].xyz[2] = z_dist *
            duration_mod;
        vaporize_effect->pertime_rand_add[0].xyz_angle[0] =
            xyz_angle * duration_mod;
        vaporize_effect->pertime_rand_add[0].xyz_angle[1] =
            xyz_angle * duration_mod;
        vaporize_effect->pertime_rand_add[0].xyz_angle[2] =
            xyz_angle * duration_mod;
        vaporize_effect->pertime_rand_add[0].bonus_rgb[0] =
            rgb_delta * duration_mod;
        vaporize_effect->pertime_rand_add[1].xyz[0] =
            xy_dist * duration_mod;
        vaporize_effect->pertime_rand_add[1].xyz[1] =
            xy_dist * duration_mod;
        vaporize_effect->pertime_rand_add[1].xyz[2] =
            z_dist * duration_mod;
        vaporize_effect->pertime_rand_add[1].xyz_angle[0] =
            -xyz_angle * duration_mod;
        vaporize_effect->pertime_rand_add[1].xyz_angle[1] =
            -xyz_angle * duration_mod;
        vaporize_effect->pertime_rand_add[1].xyz_angle[2] =
            -xyz_angle * duration_mod;
        vaporize_effect->perexptime_add.scale_factor = -1.5f;
        #endif
        
        vaporize_effect->loops = 1;
        vaporize_effect->cast_light = false;
        
        T1_particle_commit(vaporize_effect);
        #elif T1_PARTICLES_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        T1_zsprites_to_render->cpu_data[zp_i].deleted = true;
    }
    #endif
}

void T1_zsprite_anim_shatter_and_destroy(
    const int32_t object_id,
    const uint64_t duration_us)
{
    assert(0); // TODO: reimplement
    
    #if 0
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    (void)duration_us;
    #else
    #error
    #endif
    
    log_assert(duration_us > 0);
    log_assert(duration_us < 1000000000);
    log_assert(object_id >= 0);
    
    for (
        uint32_t zp_i = 0;
        zp_i < T1_zsprites_to_render->size;
        zp_i++)
    {
        if (T1_zsprites_to_render->cpu_data[zp_i].deleted ||
            !T1_zsprites_to_render->cpu_data[zp_i].committed ||
            T1_zsprites_to_render->cpu_data[zp_i].zsprite_id != object_id)
        {
            continue;
        }
        
        #if T1_PARTICLES_ACTIVE == T1_ACTIVE
        float duration_mod = (20000000.0f / (float)duration_us);
        
        T1ParticleEffect * shatter_effect = T1_particle_get_next();
        shatter_effect->zpolygon_cpu = T1_zsprites_to_render->cpu_data[zp_i];
        shatter_effect->zpolygon_gpu = T1_zsprites_to_render->gpu_data[zp_i];
        
        uint64_t shattered_verts_size =
            (uint64_t)all_mesh_summaries[shatter_effect->zpolygon_cpu.mesh_id].
                shattered_vertices_size;
        log_assert(shattered_verts_size > 0);
        shatter_effect->spawns_per_loop = (uint32_t)(
            (shattered_verts_size * 1000000) /
                (uint64_t)(duration_us + 1));
        shatter_effect->pause_per_spawn = 0;
        shatter_effect->verts_per_particle = 6;
        shatter_effect->spawn_lifespan = duration_us;
        shatter_effect->shattered = true;
        
        float xyz_dist = 0.02f;
        float xyz_angle = 0.05f;
        float rgb_delta = 0.05f;
        
        #if 0
        shatter_effect->pertime_rand_add[0].xyz[0] = -xyz_dist *
            duration_mod;
        shatter_effect->pertime_rand_add[0].xyz[1] = -xyz_dist *
            duration_mod;
        shatter_effect->pertime_rand_add[0].xyz[2] = -xyz_dist *
            duration_mod;
        shatter_effect->pertime_rand_add[0].xyz_angle[0] =
            xyz_angle * duration_mod;
        shatter_effect->pertime_rand_add[0].xyz_angle[1] =
            xyz_angle * duration_mod;
        shatter_effect->pertime_rand_add[0].xyz_angle[2] =
            xyz_angle * duration_mod;
        shatter_effect->pertime_rand_add[0].bonus_rgb[0] =
            rgb_delta * duration_mod;
        shatter_effect->pertime_rand_add[0].bonus_rgb[1] =
            rgb_delta * duration_mod;
        shatter_effect->pertime_rand_add[0].bonus_rgb[2] =
            rgb_delta * duration_mod;
        shatter_effect->pertime_rand_add[1].xyz[0] =
            xyz_dist * duration_mod;
        shatter_effect->pertime_rand_add[1].xyz[1] =
            xyz_dist * duration_mod;
        shatter_effect->pertime_rand_add[1].xyz[2] =
            xyz_dist * duration_mod;
        shatter_effect->pertime_rand_add[1].xyz_angle[0] =
            -xyz_angle * duration_mod;
        shatter_effect->pertime_rand_add[1].xyz_angle[1] =
            -xyz_angle * duration_mod;
        shatter_effect->pertime_rand_add[1].xyz_angle[2] =
            -xyz_angle * duration_mod;
        shatter_effect->perexptime_add.scale_factor = -0.07f *
            duration_mod;
        #endif
        
        shatter_effect->loops = 1;
        shatter_effect->cast_light = false;
        
        log_assert(!shatter_effect->zpolygon_cpu.alpha_blending_enabled);
        
        T1_particle_commit(shatter_effect);
        #elif T1_PARTICLES_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        T1_zsprites_to_render->cpu_data[zp_i].zsprite_id = -1;
        T1_zsprites_to_render->cpu_data[zp_i].deleted = true;
    }
    #endif
}

void T1_zsprite_anim_fade_and_destroy(
    const int32_t  object_id,
    const uint64_t duration_us)
{
    log_assert(duration_us > 0);
    
    // register scheduled animation
    T1TexQuadAnim * fade_destroy = T1_zsprite_anim_request_next(true);
    fade_destroy->affected_zsprite_id = object_id;
    fade_destroy->duration_us = duration_us;
    fade_destroy->gpu_vals.alpha = 0.0f;
    fade_destroy->delete_object_when_finished = true;
    T1_zsprite_anim_commit(fade_destroy);
}

void T1_zsprite_anim_fade_to(
    const int32_t zsprite_id,
    const uint64_t duration_us,
    const float target_alpha)
{
    log_assert(zsprite_id >= 0);
    
    // register scheduled animation
    T1TexQuadAnim * modify_alpha = T1_zsprite_anim_request_next(true);
    modify_alpha->affected_zsprite_id = zsprite_id;
    modify_alpha->duration_us = duration_us;
    modify_alpha->gpu_vals.alpha = target_alpha;
    T1_zsprite_anim_commit(modify_alpha);
}

void T1_zsprite_anim_resolve(void)
{
    tqas->mutex_lock(tqas->mutex_id);
    
    for (
        int32_t animation_i = (int32_t)texquad_anims_size - 1;
        animation_i >= 0;
        animation_i--)
    {
        T1InternalTexQuadAnim * anim =
            &texquad_anims[animation_i];
        
        if (
            anim->deleted ||
            !anim->committed)
        {
            continue;
        }
        
        if (anim->already_applied_t >= 1.0f) {
            bool32_t delete =
                anim->public.runs == 1;
            bool32_t reduce_runs =
                anim->public.runs > 0;
            
            if (delete) {
                anim->deleted = true;
                if (animation_i == (int32_t)texquad_anims_size - 1) {
                    texquad_anims_size -= 1;
                }
                
                if (
                    anim->public.
                        delete_object_when_finished)
                {
                    T1_zsprite_delete(
                        anim->public.
                            affected_zsprite_id);
                }
            } else {
                anim->remaining_duration_us =
                    anim->public.duration_us;
                anim->remaining_pause_us =
                    anim->public.pause_us;
            }
            
            if (reduce_runs) {
                anim->public.runs -= 1;
            }
            
            anim->already_applied_t = 0.0f;
            
            continue;
        }
        
        uint64_t elapsed = T1_global->elapsed;
        
        if (anim->remaining_pause_us > 0) {
            
            if (elapsed <= anim->remaining_pause_us) {
                anim->remaining_pause_us -= elapsed;
                continue;
            } else {
                elapsed -= anim->remaining_pause_us;
                anim->remaining_pause_us = 0;
            }
        }
        
        log_assert(anim->remaining_duration_us > 0);
        
        if (elapsed < anim->remaining_duration_us)
        {
            anim->remaining_duration_us -= elapsed;
        } else {
            anim->remaining_duration_us = 0;
        }
        
        uint64_t elapsed_so_far =
            anim->public.duration_us - anim->remaining_duration_us;
        log_assert(
            elapsed_so_far <=
                anim->public.duration_us);
        
        T1TPair t;
        t.now = (float)elapsed_so_far / (float)anim->public.duration_us;
        log_assert(t.now <= 1.0f);
        log_assert(t.now >= 0.0f);
        log_assert(t.now >= anim->already_applied_t);
        t.applied = anim->already_applied_t;
        
        log_assert(anim->already_applied_t <= t.now);
        anim->already_applied_t = t.now;
        
        t.now = T1_easing_t_to_eased_t(
            t.now,
            anim->public.easing_type);
        t.applied = T1_easing_t_to_eased_t(
            t.applied,
            anim->public.easing_type);
        
        // Apply effects
        for (
            int32_t zp_i = 0;
            zp_i < (int32_t)T1_zsprite_list->size;
            zp_i++)
        {
            if (
                (anim->public.affected_zsprite_id >= 0 &&
                T1_zsprite_list->cpu_data[zp_i].zsprite_id !=
                    anim->public.affected_zsprite_id) ||
                (anim->public.affected_touchable_id >= 0 &&
                T1_zsprite_list->gpu_data[zp_i].touch_id !=
                    anim->public.affected_touchable_id)
                ||
                T1_zsprite_list->cpu_data[zp_i].deleted)
            {
                continue;
            }
            
            apply_animation_effects_for_given_eased_t(
                t,
                &anim->public,
                T1_zsprite_list->gpu_data + zp_i,
                &T1_zsprite_list->cpu_data[zp_i].simd_stats);
        }
    }
    
    tqas->mutex_unlock(tqas->mutex_id);
}

void T1_zsprite_anim_dud_dance(
    const int32_t object_id,
    const float magnitude)
{
    T1TexQuadAnim * move_request =
        T1_zsprite_anim_request_next(false);
    move_request->easing_type = EASINGTYPE_QUADRUPLE_BOUNCE_ZERO_TO_ZERO;
    move_request->affected_zsprite_id = (int32_t)object_id;
    move_request->cpu_vals.xyz[0] = magnitude * 0.05f;
    move_request->cpu_vals.xyz[1] = magnitude * 0.035f;
    move_request->cpu_vals.xyz[2] = magnitude * 0.005f;
    move_request->duration_us = 300000;
    T1_zsprite_anim_commit(move_request);
}

void T1_zsprite_anim_bump(
    const int32_t object_id,
    const uint32_t wait)
{
    #if T1_LOGGER_ASSERTS_ACTIVE
    log_assert(wait == 0.0f);
    #else
    (void)wait;
    #endif
    
    T1TexQuadAnim * move_request =
        T1_zsprite_anim_request_next(false);
    move_request->easing_type = EASINGTYPE_DOUBLE_BOUNCE_ZERO_TO_ZERO;
    move_request->affected_zsprite_id = (int32_t)object_id;
    move_request->cpu_vals.scale_factor = 0.25f;
    move_request->duration_us = 200000;
    T1_zsprite_anim_commit(
        move_request);
}

void T1_zsprite_anim_delete_all(void)
{
    tqas->mutex_lock(tqas->mutex_id);
    for (uint32_t i = 0; i < texquad_anims_size; i++) {
        texquad_anims[i].deleted = true;
    }
    tqas->mutex_unlock(tqas->mutex_id);
}

void T1_zsprite_anim_delete_endpoint_anims_targeting(
    const int32_t object_id)
{
    tqas->mutex_lock(tqas->mutex_id);
    for (uint32_t i = 0; i < texquad_anims_size; i++) {
        if (
            !texquad_anims[i].deleted &&
            texquad_anims[i].endpoints_not_deltas &&
            texquad_anims[i].public.
                affected_zsprite_id ==
                    (int32_t)object_id)
        {
            texquad_anims[i].deleted = true;
        }
    }
    tqas->mutex_unlock(tqas->mutex_id);
}

void T1_zsprite_anim_delete_all_anims_targeting(
    const int32_t object_id)
{
    tqas->mutex_lock(tqas->mutex_id);
    for (uint32_t i = 0; i < texquad_anims_size; i++) {
        T1TexQuadAnim * a =
            &texquad_anims[i].public;
        
        if (
            !texquad_anims[i].deleted &&
            texquad_anims[i].committed &&
            a->affected_zsprite_id == object_id)
        {
            texquad_anims[i].deleted = true;
        }
    }
    tqas->mutex_unlock(tqas->mutex_id);
}

void T1_zsprite_anim_set_ignore_camera_but_retain_screenspace_pos(
    const int32_t zsprite_id,
    const float new_ignore_camera)
{
    T1GPUTexQuad * zs = NULL;
    T1CPUzSprite * zs_cpu = NULL;
    
    for (uint32_t i = 0; i < T1_zsprite_list->size; i++)
    {
        if (T1_zsprite_list->cpu_data[i].zsprite_id == zsprite_id) {
            zs = T1_zsprite_list->gpu_data + i;
            zs_cpu = T1_zsprite_list->cpu_data + i;
        }
    }
    
    if (zs->ignore_camera == new_ignore_camera)
    {
        return;
    }
    
    // For now we're only supporting the easy case of a full toggle
    bool32_t is_near_zero =
        zs->ignore_camera > -0.01f &&
        zs->ignore_camera <  0.01f;
    #if T1_LOGGER_ASSERTS_ACTIVE
    bool32_t is_near_one =
        zs->ignore_camera >  0.99f &&
        zs->ignore_camera <  1.01f;
    #endif
    log_assert(is_near_zero || is_near_one);
    
    if (is_near_zero) {
        log_assert(new_ignore_camera == 1.0f);
        
        zs_cpu->simd_stats.xyz[0] -= T1_camera->xyz[0];
        zs_cpu->simd_stats.xyz[1] -= T1_camera->xyz[1];
        zs_cpu->simd_stats.xyz[2] -= T1_camera->xyz[2];
        x_rotate_f3(zs_cpu->simd_stats.xyz, -T1_camera->xyz_angle[0]);
        y_rotate_f3(zs_cpu->simd_stats.xyz, -T1_camera->xyz_angle[1]);
        z_rotate_f3(zs_cpu->simd_stats.xyz, -T1_camera->xyz_angle[2]);
        
        #if 1
        // This is a hack, an approximation
        zs_cpu->simd_stats.angle_xyz[0] -= T1_camera->xyz_angle[0];
        zs_cpu->simd_stats.angle_xyz[1] -= T1_camera->xyz_angle[1];
        zs_cpu->simd_stats.angle_xyz[2] -= T1_camera->xyz_angle[2];
        #endif
        
        zs->ignore_camera = 1.0f;
    } else {
        log_assert(is_near_one);
        
        z_rotate_f3(zs_cpu->simd_stats.xyz, T1_camera->xyz_angle[2]);
        y_rotate_f3(zs_cpu->simd_stats.xyz, T1_camera->xyz_angle[1]);
        x_rotate_f3(zs_cpu->simd_stats.xyz, T1_camera->xyz_angle[0]);
        
        zs_cpu->simd_stats.xyz[0] += T1_camera->xyz[0];
        zs_cpu->simd_stats.xyz[1] += T1_camera->xyz[1];
        zs_cpu->simd_stats.xyz[2] += T1_camera->xyz[2];
        
        #if 1
        // This is a hack, an approximation
        zs_cpu->simd_stats.angle_xyz[0] += T1_camera->xyz_angle[0];
        zs_cpu->simd_stats.angle_xyz[1] += T1_camera->xyz_angle[1];
        zs_cpu->simd_stats.angle_xyz[2] += T1_camera->xyz_angle[2];
        #endif
        
        zs->ignore_camera = 0.0f;
    }
}

#elif T1_TEXQUAD_ANIM_ACTIVE == T1_INACTIVE
#else
#error
#endif // T1_SCHEDULED_ANIMS_ACTIVE
