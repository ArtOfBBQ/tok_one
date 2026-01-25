#include "T1_zsprite_anim.h"

#if T1_ZSPRITE_ANIM_ACTIVE == T1_ACTIVE

typedef struct {
    T1zSpriteAnim public;
    
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
} T1InternalzSpriteAnim;

T1InternalzSpriteAnim * T1_zsprite_anims = NULL;
uint32_t zsprite_anims_size = 0;

typedef struct {
    T1GPUzSprite zsprite_final_pos_gpu;
    T1GPUzSprite zsprite_deltas[2];
    T1CPUzSpriteSimdStats zsprite_final_pos_cpu;
    uint32_t (* init_mutex_and_return_id)(void);
    void (* mutex_lock)(const uint32_t);
    void (* mutex_unlock)(const uint32_t);
    uint32_t mutex_id;
} T1FrameAnimState;

static T1FrameAnimState * fas = NULL;

void T1_zsprite_anim_init(
    uint32_t (* funcptr_init_mutex_and_return_id)(void),
    void (* funcptr_mutex_lock)(const uint32_t),
    void (* funcptr_mutex_unlock)(const uint32_t))
{
    T1_zsprite_anims = (T1InternalzSpriteAnim *)
        T1_mem_malloc_from_unmanaged(
            sizeof(
                T1InternalzSpriteAnim) *
                    T1_ZSPRITE_ANIMS_CAP);
    T1_std_memset(
        T1_zsprite_anims,
        0,
        sizeof(T1InternalzSpriteAnim));
    T1_zsprite_anims[0].deleted = true;
    
    fas = (T1FrameAnimState *)T1_mem_malloc_from_unmanaged(
        sizeof(T1FrameAnimState));
    T1_std_memset(
        fas,
        0,
        sizeof(T1FrameAnimState));
    
    fas->init_mutex_and_return_id =
        funcptr_init_mutex_and_return_id;
    fas->mutex_lock = funcptr_mutex_lock;
    fas->mutex_unlock = funcptr_mutex_unlock;
    
    for (
        uint32_t i = 1;
        i < T1_ZSPRITE_ANIMS_CAP;
        i++)
    {
        T1_std_memcpy(
            &T1_zsprite_anims[i],
            &T1_zsprite_anims[0],
            sizeof(T1InternalzSpriteAnim));
        log_assert(T1_zsprite_anims[i].deleted);
    }
    
    fas->mutex_id = fas->init_mutex_and_return_id();
}

static
T1InternalzSpriteAnim * T1_zsprite_anim_get_container(
    const T1zSpriteAnim * public_ptr)
{
    assert(
        offsetof(T1InternalzSpriteAnim, public) == 0);
    
    T1InternalzSpriteAnim * retval = (T1InternalzSpriteAnim *)public_ptr;
    
    log_assert(
        &retval->public == public_ptr);
    
    return retval;
}

static void construct_scheduled_animationA(
    T1InternalzSpriteAnim * to_construct)
{
    T1_std_memset(
        to_construct,
        0,
        sizeof(T1InternalzSpriteAnim));
    log_assert(!to_construct->committed);
    
    to_construct->public.affected_zsprite_id = -1;
    to_construct->public.affected_touch_id = -1;
    to_construct->public.runs = 1;
    
    log_assert(!to_construct->deleted);
    log_assert(!to_construct->committed);
}

T1zSpriteAnim * T1_zsprite_anim_request_next(
    bool8_t endpoints_not_deltas)
{
    fas->mutex_lock(fas->mutex_id);
    
    log_assert(
        zsprite_anims_size < T1_ZSPRITE_ANIMS_CAP);
    T1InternalzSpriteAnim * return_value = NULL;
    
    for (
        uint32_t i = 0;
        i < zsprite_anims_size;
        i++)
    {
        if (T1_zsprite_anims[i].deleted)
        {
            return_value = &T1_zsprite_anims[i];
        }
    }
    
    if (return_value == NULL) {
        return_value =
            &T1_zsprite_anims[zsprite_anims_size];
        zsprite_anims_size += 1;
        log_assert(
            zsprite_anims_size <
                T1_ZSPRITE_ANIMS_CAP);
    }
    
    log_assert(return_value->deleted);
    construct_scheduled_animationA(return_value);
    
    log_assert(!return_value->committed);
    log_assert(!return_value->deleted);
    
    if (endpoints_not_deltas) {
        T1_std_memset_f32(
            &return_value->public.gpu_vals,
            T1_ZSPRITEANIM_NO_EFFECT,
            sizeof(T1GPUzSprite));
        T1_std_memset_f32(
            &return_value->public.cpu_vals,
            T1_ZSPRITEANIM_NO_EFFECT,
            sizeof(T1CPUzSpriteSimdStats));
        return_value->endpoints_not_deltas = endpoints_not_deltas;
    }
    
    log_assert(!return_value->committed);
    log_assert(!return_value->deleted);
    
    fas->mutex_unlock(fas->mutex_id);
    
    return &return_value->public;
}

static void T1_zsprite_anim_resolve_single(
    T1InternalzSpriteAnim * anim)
{
    if (
        anim->deleted ||
        !anim->committed)
    {
        return;
    }
    
    if (anim->already_applied_t >= 1.0f) {
        bool32_t delete =
            anim->public.runs == 1;
        bool32_t reduce_runs =
            anim->public.runs > 0;
        
        if (delete) {
            anim->deleted = true;
            
            if (
                anim->public.
                    del_obj_on_finish)
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
        
        return;
    }
    
    uint64_t elapsed = T1_global->elapsed;
    
    if (anim->remaining_pause_us > 0) {
        
        if (elapsed <= anim->remaining_pause_us) {
            anim->remaining_pause_us -= elapsed;
            return;
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
    log_assert(
        t.now >= anim->already_applied_t);
    t.applied = anim->already_applied_t;
    
    log_assert(anim->already_applied_t <= t.now);
    anim->already_applied_t = t.now;
    
    t.now = T1_easing_t_to_eased_t(
        t.now,
        anim->public.easing_type);
    t.applied = T1_easing_t_to_eased_t(
        t.applied,
        anim->public.easing_type);
    
    if (anim->endpoints_not_deltas) {
        
        T1_zsprite_apply_endpoint_anim(
            /* const int32_t zsprite_id: */
                anim->public.
                    affected_zsprite_id,
            /* const int32_t touch_id: */
                anim->public.affected_touch_id,
            /* const float t_applied: */
                t.applied,
            /* const float t_now: */
                t.now,
            /* const float * goal_gpu_vals_f32: */
                anim->public.gpu_vals_f32_active ?
                    (float *)&anim->public.gpu_vals.f32 :
                    NULL,
            /* const int32_t *goal_gpu_vals_i32: */
                anim->public.gpu_vals_i32_active ? (int32_t *)&anim->public.gpu_vals.i32 :
                NULL,
            /* const float * goal_cpu_vals: */
                anim->public.cpu_vals_active ?
                    (float *)&anim->public.cpu_vals :
                    NULL);
    } else {
        T1_zsprite_apply_anim_effects_to_id(
            /* const int32_t zsprite_id: */
                anim->public.affected_zsprite_id,
            /* const int32_t touch_id: */
                anim->public.affected_touch_id,
            /* const float t_applied: */
                t.applied,
            /* const float t_now: */
                t.now,
            /* const float * anim_gpu_vals_f32: */
                anim->public.gpu_vals_f32_active ?
                    (float *)&anim->public.gpu_vals.f32 :
                    NULL,
            /* const int32_t * anim_gpu_vals_i32: */
                anim->public.gpu_vals_i32_active ?
                    (int32_t *)&anim->public.gpu_vals.i32 :
                    NULL,
            /* const float * anim_cpu_vals: */
                anim->public.cpu_vals_active ?
                (float *)&anim->public.cpu_vals :
                NULL);
    }
}

void T1_zsprite_anim_commit_and_instarun(
    T1zSpriteAnim * to_commit)
{
    fas->mutex_lock(fas->mutex_id);
    
    T1InternalzSpriteAnim * parent =
        T1_zsprite_anim_get_container(to_commit);
    log_assert(&parent->public == to_commit);
    log_assert(to_commit->duration_us == 1);
    
    parent->remaining_pause_us =
        to_commit->pause_us;
    parent->remaining_duration_us =
        to_commit->duration_us;
    parent->committed = true;
    
    T1_zsprite_anim_resolve_single(parent);
    parent->deleted = true;
    parent->committed = false;
    log_assert(parent->remaining_duration_us == 0);
    
    fas->mutex_unlock(fas->mutex_id);
}

void
T1_zsprite_anim_assert_anim_valid_before_commit(
    T1zSpriteAnim * to_check)
{
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    T1InternalzSpriteAnim * parent =
        T1_zsprite_anim_get_container(to_check);
    log_assert(&parent->public == to_check);
    
    if (parent->endpoints_not_deltas) {
        
        if (!parent->public.gpu_vals_f32_active) {
            for (
                uint32_t f_i = 0;
                f_i < (sizeof(T1GPUzSpritef32) / 4);
                f_i++)
            {
                log_assert(
                    ((float *)&parent->
                        public.gpu_vals.f32)[f_i] ==
                    T1_ZSPRITEANIM_NO_EFFECT);
            }
        }
        if (!parent->public.gpu_vals_i32_active) {
            for (
                uint32_t f_i = 0;
                f_i < (sizeof(T1GPUzSpritei32) / 4);
                f_i++)
            {
                log_assert(
                    ((float *)&parent->
                        public.gpu_vals.i32)[f_i] ==
                    T1_ZSPRITEANIM_NO_EFFECT);
            }
        } else {
            uint32_t nonskips = 0;
            for (
                uint32_t f_i = 0;
                f_i < (sizeof(T1GPUzSpritei32) / 4);
                f_i++)
            {
                if (
                    ((float *)&parent->
                        public.gpu_vals.i32)[f_i] !=
                    T1_ZSPRITEANIM_NO_EFFECT)
                {
                    nonskips += 1;
                }
            }
            
            // gpu_vals_i32_active but useless
            log_assert(nonskips > 0);
            
            switch (parent->public.easing_type) {
                case EASINGTYPE_ALWAYS_1:
                    // this is always fine
                break;
                default:
                    // avoid int->float casting issues
                    log_assert(parent->public.duration_us == 1);
            }
        }
        if (!parent->public.cpu_vals_active) {
            for (
                uint32_t f_i = 0;
                f_i < (sizeof(T1CPUzSpriteSimdStats) / 4);
                f_i++)
            {
                log_assert(
                    ((float *)&parent->
                        public.cpu_vals)[f_i] ==
                    T1_ZSPRITEANIM_NO_EFFECT);
            }
        }
        
        uint32_t skips = 0;
        
        float * vals = (float *)
            (&parent->public.gpu_vals);
        
        for (
            uint32_t i = 0;
            i < sizeof(T1GPUzSprite) / 4;
            i += SIMD_FLOAT_LANES)
        {
            for (
                uint32_t j = 0;
                j < SIMD_FLOAT_LANES;
                j++)
            {
                skips += vals[i+j] ==
                    T1_ZSPRITEANIM_NO_EFFECT;
            }
        }
        
        log_assert(skips > 0);
    } else {
        if (!parent->public.gpu_vals_f32_active) {
            for (
                uint32_t f_i = 0;
                f_i < (sizeof(T1GPUzSpritef32) / 4);
                f_i++)
            {
                log_assert(
                    ((float *)&parent->
                        public.gpu_vals.f32)[f_i] ==
                    0.0f);
            }
        }
        if (!parent->public.gpu_vals_i32_active) {
            for (
                uint32_t f_i = 0;
                f_i < (sizeof(T1GPUzSpritei32) / 4);
                f_i++)
            {
                log_assert(
                    ((int32_t *)&parent->
                        public.gpu_vals.i32)[f_i] ==
                    0);
            }
        }
        if (!parent->public.cpu_vals_active) {
            for (
                uint32_t f_i = 0;
                f_i < (sizeof(T1CPUzSpriteSimdStats) / 4);
                f_i++)
            {
                log_assert(
                    ((float *)&parent->
                        public.cpu_vals)[f_i] ==
                    0.0f);
            }
        }
    }
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    log_assert(to_check->duration_us > 0);
}

void T1_zsprite_anim_commit(
    T1zSpriteAnim * to_commit)
{
    fas->mutex_lock(fas->mutex_id);
    
    T1_zsprite_anim_assert_anim_valid_before_commit(to_commit);
    
    T1InternalzSpriteAnim * parent =
        T1_zsprite_anim_get_container(to_commit);
    log_assert(&parent->public == to_commit);
    
    if (to_commit->del_conflict_anims)
    {
        for (
            uint32_t anim_i = 0;
            anim_i < zsprite_anims_size;
            anim_i++)
        {
            T1InternalzSpriteAnim * a =
                &T1_zsprite_anims[anim_i];
            
            if (
                a->public.affected_zsprite_id ==
                    to_commit->affected_zsprite_id &&
                a->public.affected_touch_id ==
                    to_commit->affected_touch_id &&
                a->committed &&
                a->endpoints_not_deltas)
            {
                T1_zsprite_anims[anim_i].deleted = true;
            }
        }
    }
    
    log_assert(!parent->deleted);
    log_assert(!parent->committed);
    
    if (to_commit->affected_zsprite_id < 0) {
        log_assert(to_commit->affected_touch_id >= 0);
    } else {
        log_assert(
            to_commit->affected_touch_id == -1);
    }
    
    if (to_commit->affected_touch_id < 0) {
        log_assert(
            to_commit->affected_zsprite_id >= 0);
    } else {
        log_assert(
            to_commit->affected_zsprite_id == -1);
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
    fas->mutex_unlock(fas->mutex_id);
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
        shatter_effect->spawns_per_loop =
            (uint32_t)(
            (shattered_verts_size * 1000000) /
                (uint64_t)(duration_us + 1));
        shatter_effect->pause_per_spawn = 0;
        shatter_effect->verts_per_particle = 6;
        shatter_effect->spawn_lifespan =
            duration_us;
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
    T1zSpriteAnim * fade_destroy = T1_zsprite_anim_request_next(true);
    fade_destroy->affected_zsprite_id = object_id;
    fade_destroy->duration_us = duration_us;
    fade_destroy->gpu_vals.f32.alpha = 0.0f;
    fade_destroy->del_obj_on_finish = true;
    fade_destroy->gpu_vals_f32_active = true;
    T1_zsprite_anim_commit(fade_destroy);
}

void T1_zsprite_anim_fade_to(
    const int32_t zsprite_id,
    const uint64_t duration_us,
    const float target_alpha)
{
    log_assert(zsprite_id >= 0);
    
    // register scheduled animation
    T1zSpriteAnim * modify_alpha = T1_zsprite_anim_request_next(true);
    modify_alpha->affected_zsprite_id = zsprite_id;
    modify_alpha->duration_us = duration_us;
    modify_alpha->gpu_vals.f32.alpha = target_alpha;
    modify_alpha->gpu_vals_f32_active = true;
    T1_zsprite_anim_commit(modify_alpha);
}

void T1_zsprite_anim_resolve(void)
{
    fas->mutex_lock(fas->mutex_id);
    
    for (
        int32_t animation_i = (int32_t)zsprite_anims_size - 1;
        animation_i >= 0;
        animation_i--)
    {
        T1InternalzSpriteAnim * anim =
            &T1_zsprite_anims[animation_i];
        
        T1_zsprite_anim_resolve_single(anim);
    }
    
    fas->mutex_unlock(fas->mutex_id);
}

void T1_zsprite_anim_dud_dance(
    const int32_t object_id,
    const float magnitude)
{
    T1zSpriteAnim * move_request =
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
    
    T1zSpriteAnim * bump_request =
        T1_zsprite_anim_request_next(false);
    bump_request->easing_type = EASINGTYPE_DOUBLE_BOUNCE_ZERO_TO_ZERO;
    bump_request->affected_zsprite_id = (int32_t)object_id;
    bump_request->cpu_vals.mul_xyz[0] = 0.05f;
    bump_request->cpu_vals.mul_xyz[1] = 0.05f;
    bump_request->cpu_vals.mul_xyz[2] = 0.05f;
    bump_request->duration_us = 200000;
    T1_zsprite_anim_commit(bump_request);
}

void T1_zsprite_anim_delete_all(void)
{
    fas->mutex_lock(fas->mutex_id);
    for (uint32_t i = 0; i < zsprite_anims_size; i++) {
        T1_zsprite_anims[i].deleted = true;
    }
    fas->mutex_unlock(fas->mutex_id);
}

void T1_zsprite_anim_delete_endpoint_anims_targeting(
    const int32_t object_id)
{
    fas->mutex_lock(fas->mutex_id);
    for (uint32_t i = 0; i < zsprite_anims_size; i++) {
        if (
            !T1_zsprite_anims[i].deleted &&
            T1_zsprite_anims[i].endpoints_not_deltas &&
            T1_zsprite_anims[i].public.
                affected_zsprite_id ==
                    (int32_t)object_id)
        {
            T1_zsprite_anims[i].deleted = true;
        }
    }
    fas->mutex_unlock(fas->mutex_id);
}

void T1_zsprite_anim_delete_all_anims_targeting(
    const int32_t object_id)
{
    fas->mutex_lock(fas->mutex_id);
    for (uint32_t i = 0; i < zsprite_anims_size; i++) {
        T1zSpriteAnim * a =
            &T1_zsprite_anims[i].public;
        
        if (
            !T1_zsprite_anims[i].deleted &&
            T1_zsprite_anims[i].committed &&
            a->affected_zsprite_id == object_id)
        {
            T1_zsprite_anims[i].deleted = true;
        }
    }
    fas->mutex_unlock(fas->mutex_id);
}

#elif T1_ZSPRITE_ANIM_ACTIVE == T1_INACTIVE
#else
#error
#endif // T1_SCHEDULED_ANIMS_ACTIVE
