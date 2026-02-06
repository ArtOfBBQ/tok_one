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
        T1_mem_malloc_unmanaged(
            sizeof(
                T1InternalzSpriteAnim) *
                    T1_ZSPRITE_ANIMS_CAP);
    T1_std_memset(
        T1_zsprite_anims,
        0,
        sizeof(T1InternalzSpriteAnim));
    T1_zsprite_anims[0].deleted = true;
    
    fas = (T1FrameAnimState *)T1_mem_malloc_unmanaged(
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
T1InternalzSpriteAnim *
T1_zsprite_anim_get_container(
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
        bool32_t delete = anim->public.runs == 1;
        bool32_t reduce_runs =
            anim->public.runs > 0;
        
        if (delete) {
            anim->deleted = true;
            
            if (anim->public.del_obj_on_finish)
            {
                if (
                    anim->public.
                        affected_zsprite_id ==
                    T1_ZSPRITE_ID_HIT_EVERYTHING)
                {
                    T1_zsprite_delete_all();
                } else {
                    T1_zsprite_delete(
                        anim->public.
                            affected_zsprite_id);
                }
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
    
    float t_now = (float)elapsed_so_far / (float)anim->public.duration_us;
    log_assert(t_now <= 1.0f);
    log_assert(t_now >= 0.0f);
    log_assert(
        t_now >= anim->already_applied_t);
    float t_applied = anim->already_applied_t;
    
    log_assert(anim->already_applied_t <= t_now);
    anim->already_applied_t = t_now;
    
    t_now = T1_easing_t_to_eased_t(
        t_now,
        anim->public.easing_type);
    t_applied = T1_easing_t_to_eased_t(
        t_applied,
        anim->public.easing_type);
    
    if (anim->endpoints_not_deltas) {
        
        T1_zsprite_apply_endpoint_anim(
            /* const int32_t zsprite_id: */
                anim->public.
                    affected_zsprite_id,
            /* const int32_t touch_id: */
                anim->public.affected_touch_id,
            /* const float t_applied: */
                t_applied,
            /* const float t_now: */
                t_now,
            /* const float * goal_gpu_vals_f32: */
                anim->public.gpu_vals_f32_active ?
                    (float *)&anim->public.gpu_vals.f32 :
                    NULL,
            /* const int32_t * goal_gpu_vals_i32: */
                anim->public.gpu_vals_i32_active ?
                    (int32_t *)&anim->
                        public.gpu_vals.i32 :
                NULL,
            /* const float * goal_cpu_vals: */
                anim->public.cpu_vals_active ?
                    (float *)&anim->
                        public.cpu_vals :
                    NULL);
    } else {
        T1_zsprite_apply_anim_effects_to_id(
            /* const int32_t zsprite_id: */
                anim->public.affected_zsprite_id,
            /* const int32_t touch_id: */
                anim->public.affected_touch_id,
            /* const float t_applied: */
                t_applied,
            /* const float t_now: */
                t_now,
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
    
    T1_zsprite_anim_assert_anim_valid_before_commit(to_commit);
    
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
        } else {
            // don't reduce alpha by 1 to fade out, use
            // an endpoint anim
            log_assert(parent->public.gpu_vals.f32.alpha > -0.9f);
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
                (a->public.affected_zsprite_id ==
                    to_commit->affected_zsprite_id ||
                a->public.affected_zsprite_id ==
                    T1_ZSPRITE_ID_HIT_EVERYTHING) &&
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

void T1_zsprite_anim_shatter_and_destroy(
    const int32_t zsprite_id,
    const uint64_t duration_us)
{
    #if T1_LOGGER_ASSERTS_ACTIVE
    #else
    (void)duration_us;
    #endif
    
    log_assert(duration_us > 0);
    log_assert(zsprite_id >= 0);
    
    T1zSpriteAnim * set_scatter_mesh =
        T1_zsprite_anim_request_next(true);
    set_scatter_mesh->affected_zsprite_id = zsprite_id;
    set_scatter_mesh->cpu_vals.alpha_blending_on = 1.0f;
    set_scatter_mesh->cpu_vals_active = true;
    set_scatter_mesh->duration_us = 1;
    T1_zsprite_anim_commit_and_instarun(set_scatter_mesh);
    
    T1zSpriteAnim * scatter = T1_zsprite_anim_request_next(true);
    scatter->gpu_vals.f32.explode = 1.25f;
    scatter->gpu_vals.f32.alpha = 0.0f;
    scatter->affected_zsprite_id = zsprite_id;
    scatter->duration_us = duration_us;
    scatter->easing_type = EASINGTYPE_NONE;
    scatter->runs = 1;
    scatter->del_obj_on_finish = true;
    scatter->gpu_vals_f32_active = true;
    T1_zsprite_anim_commit(scatter);
}

void T1_zsprite_anim_evaporate_and_destroy(
    const int32_t zsprite_id,
    const uint64_t duration_us)
{
    #if T1_LOGGER_ASSERTS_ACTIVE
    #else
    (void)duration_us;
    #endif
    
    log_assert(duration_us > 0);
    log_assert(zsprite_id >= 0);
    
    T1zSpriteAnim * set_scatter_mesh =
        T1_zsprite_anim_request_next(true);
    set_scatter_mesh->affected_zsprite_id = zsprite_id;
    set_scatter_mesh->cpu_vals.alpha_blending_on = 1.0f;
    set_scatter_mesh->cpu_vals_active = true;
    set_scatter_mesh->duration_us = 1;
    T1_zsprite_anim_commit_and_instarun(set_scatter_mesh);
    
    T1zSpriteAnim * evap =
        T1_zsprite_anim_request_next(true);
    evap->gpu_vals.f32.explode = 0.04f;
    evap->gpu_vals.f32.alpha = 0.0f;
    evap->affected_zsprite_id = zsprite_id;
    evap->duration_us = duration_us;
    evap->easing_type = EASINGTYPE_NONE;
    evap->runs = 1;
    evap->del_obj_on_finish = true;
    evap->gpu_vals_f32_active = true;
    T1_zsprite_anim_commit(evap);
}

void T1_zsprite_anim_fade_and_destroy(
    const int32_t  object_id,
    const uint64_t duration_us)
{
    log_assert(duration_us > 0);
    
    // register scheduled animation
    T1zSpriteAnim * fade_destroy =
        T1_zsprite_anim_request_next(true);
    fade_destroy->affected_zsprite_id = object_id;
    fade_destroy->duration_us = duration_us;
    fade_destroy->gpu_vals.f32.alpha = 0.0f;
    fade_destroy->del_obj_on_finish = true;
    fade_destroy->gpu_vals_f32_active = true;
    T1_zsprite_anim_commit(fade_destroy);
}

void T1_zsprite_anim_fade_destroy_all(
    const uint64_t duration_us)
{
    T1_zsprite_anim_fade_and_destroy(
        /* const int32_t  object_id: */
            T1_ZSPRITE_ID_HIT_EVERYTHING,
        /* const uint64_t duration_us: */
            duration_us);
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
    bump_request->cpu_vals_active = true;
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
            (T1_zsprite_anims[i].public.
                affected_zsprite_id ==
                    (int32_t)object_id ||
            T1_zsprite_anims[i].
                public.affected_zsprite_id ==
                    T1_ZSPRITE_ID_HIT_EVERYTHING))
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
            (a->affected_zsprite_id == object_id ||
            a->affected_zsprite_id == T1_ZSPRITE_ID_HIT_EVERYTHING))
        {
            T1_zsprite_anims[i].deleted = true;
        }
    }
    fas->mutex_unlock(fas->mutex_id);
}

#elif T1_ZSPRITE_ANIM_ACTIVE == T1_INACTIVE
#else
#error
#endif // T1_ZSPRITE_ANIM_ACTIVE
