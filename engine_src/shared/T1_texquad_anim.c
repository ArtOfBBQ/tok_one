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
    
    bool8_t endpoints;
    
    bool8_t deleted;
    bool8_t committed;
} T1InternalTexQuadAnim;

#define FLT_TQDANIM_IGNORE 0xFFFF

typedef struct {
    T1InternalTexQuadAnim anims[T1_TEXQUAD_ANIMS_CAP];
    uint32_t (* init_mutex_and_return_id)(void);
    void (* mutex_lock)(const uint32_t);
    void (* mutex_unlock)(const uint32_t);
    uint32_t anims_size;
    uint32_t mutex_id;
} T1TexQuadAnimState;

static T1TexQuadAnimState * tqas = NULL;

void T1_texquad_anim_init(
    uint32_t (* funcptr_init_mutex_and_return_id)(void),
    void (* funcptr_mutex_lock)(const uint32_t),
    void (* funcptr_mutex_unlock)(const uint32_t))
{
    tqas = (T1TexQuadAnimState *)
        T1_mem_malloc_from_unmanaged(
            sizeof(T1TexQuadAnimState));
    T1_std_memset(
        tqas,
        0,
        sizeof(T1TexQuadAnimState));
    
    tqas->init_mutex_and_return_id =
        funcptr_init_mutex_and_return_id;
    tqas->mutex_lock = funcptr_mutex_lock;
    tqas->mutex_unlock = funcptr_mutex_unlock;
    
    tqas->anims[0].deleted = true;
    
    for (
        uint32_t i = 1;
        i < T1_TEXQUAD_ANIMS_CAP;
        i++)
    {
        T1_std_memcpy(
            &tqas->anims[i],
            &tqas->anims[0],
            sizeof(T1InternalTexQuadAnim));
        log_assert(tqas->anims[i].deleted);
    }
    
    tqas->mutex_id = tqas->
        init_mutex_and_return_id();
}

static
T1InternalTexQuadAnim * T1_texquad_anim_get_container(
    const T1TexQuadAnim * public_ptr)
{
    log_assert(
        offsetof(
            T1InternalTexQuadAnim,
            public) == 0);
    
    T1InternalTexQuadAnim * retval =
        (T1InternalTexQuadAnim *)
            public_ptr;
    
    log_assert(
        &retval->public == public_ptr);
    
    return retval;
}

static void T1_texquad_anim_construct(
    T1InternalTexQuadAnim * to_construct)
{
    T1_std_memset(
        to_construct,
        0,
        sizeof(T1InternalTexQuadAnim));
    log_assert(
        !to_construct->committed);
    
    to_construct->public.
        affect_zsprite_id = -1;
    to_construct->public.
        affect_touch_id = -1;
    to_construct->public.runs = 1;
    
    log_assert(
        !to_construct->deleted);
    log_assert(
        !to_construct->committed);
}

T1TexQuadAnim * T1_texquad_anim_request_next(
    bool8_t endpoints_not_deltas)
{
    tqas->mutex_lock(tqas->mutex_id);
    
    log_assert(
        tqas->anims_size <
            T1_TEXQUAD_ANIMS_CAP);
    T1InternalTexQuadAnim * return_value = NULL;
    
    for (
        uint32_t i = 0;
        i < tqas->anims_size;
        i++)
    {
        if (tqas->anims[i].deleted)
        {
            return_value = &tqas->anims[i];
        }
    }
    
    if (return_value == NULL) {
        return_value =
            &tqas->anims[tqas->anims_size];
        tqas->anims_size += 1;
        log_assert(
            tqas->anims_size <
                T1_TEXQUAD_ANIMS_CAP);
    }
    
    log_assert(return_value->deleted);
    T1_texquad_anim_construct(
        return_value);
    
    log_assert(!return_value->committed);
    log_assert(!return_value->deleted);
    
    if (endpoints_not_deltas) {
        T1_std_memset_f32(
            &return_value->public.gpu_vals,
            T1_TEXQUADANIM_NO_EFFECT,
            sizeof(T1GPUTexQuad));
        return_value->endpoints = endpoints_not_deltas;
    }
    
    log_assert(!return_value->committed);
    log_assert(!return_value->deleted);
    
    tqas->mutex_unlock(tqas->mutex_id);
    
    return &return_value->public;
}

static void T1_texquad_anim_resolve_single(
    T1InternalTexQuadAnim * anim)
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
                if (
                    anim->public.affect_zsprite_id ==
                    T1_TEXQUAD_ID_HIT_EVERYTHING)
                {
                    T1_texquad_delete_all();
                } else {
                    T1_texquad_delete(
                        anim->public.
                            affect_zsprite_id);
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
    
    if (anim->endpoints) {
        
        T1_texquad_apply_endpoint_anim(
            /* const int32_t zsprite_id: */
                anim->public.
                    affect_zsprite_id,
            /* const int32_t touch_id: */
                anim->public.affect_touch_id,
            /* const float t_applied: */
                t.applied,
            /* const float t_now: */
                t.now,
            /* const float * goal_gpu_vals_f32: */
                anim->public.gpu_f32_active ?
                    (float *)
                        &anim->public.gpu_vals.f32 :
                    NULL,
            /* const int32_t * goal_gpu_vals_i32: */
                anim->public.gpu_i32_active ? (int32_t *)&anim->public.gpu_vals.i32 :
                NULL);
    } else {
        T1_texquad_apply_anim_effects_to_id(
            /* const int32_t zsprite_id: */
                anim->public.affect_zsprite_id,
            /* const int32_t touch_id: */
                anim->public.affect_touch_id,
            /* const float t_applied: */
                t.applied,
            /* const float t_now: */
                t.now,
            /* const float * anim_gpu_vals_f32: */
                anim->public.gpu_f32_active ?
                    (float *)&anim->
                        public.gpu_vals.f32 :
                    NULL,
            /* const int32_t * anim_gpu_vals_i32: */
                anim->public.gpu_i32_active ?
                    (int32_t *)&anim->
                        public.gpu_vals.i32 :
                    NULL);
    }
}

void T1_texquad_anim_commit(
    T1TexQuadAnim * to_commit)
{
    tqas->mutex_lock(tqas->mutex_id);
    
    T1InternalTexQuadAnim * parent =
        T1_texquad_anim_get_container(to_commit);
    log_assert(&parent->public == to_commit);
    
    if (to_commit->del_conflict_anims)
    {
        for (
            uint32_t anim_i = 0;
            anim_i < tqas->anims_size;
            anim_i++)
        {
            T1InternalTexQuadAnim * a =
                &tqas->anims[anim_i];
            
            if (
                a->public.affect_zsprite_id ==
                    to_commit->affect_zsprite_id &&
                a->public.affect_touch_id ==
                    to_commit->affect_touch_id &&
                a->committed &&
                !a->public.del_obj_on_finish &&
                a->endpoints)
            {
                tqas->anims[anim_i].
                    deleted = true;
            }
        }
    }
    
    log_assert(!parent->deleted);
    log_assert(!parent->committed);
    
    if (to_commit->affect_zsprite_id < 0) {
        log_assert(to_commit->affect_touch_id >= 0);
    } else {
        log_assert(
            to_commit->affect_touch_id == -1);
    }
    
    if (to_commit->affect_touch_id < 0) {
        log_assert(
            to_commit->affect_zsprite_id >= 0);
    } else {
        log_assert(
            to_commit->affect_zsprite_id == -1);
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

void T1_texquad_anim_commit_and_instarun(
    T1TexQuadAnim * to_commit)
{
    tqas->mutex_lock(tqas->mutex_id);
    
    T1InternalTexQuadAnim * parent =
        T1_texquad_anim_get_container(to_commit);
    log_assert(&parent->public == to_commit);
    log_assert(to_commit->duration_us == 1);
    
    parent->remaining_pause_us =
        to_commit->pause_us;
    parent->remaining_duration_us =
        to_commit->duration_us;
    parent->committed = true;
    
    T1_texquad_anim_resolve_single(parent);
    parent->deleted = true;
    parent->committed = false;
    log_assert(parent->remaining_duration_us == 0);
    
    tqas->mutex_unlock(tqas->mutex_id);
}

void T1_texquad_anim_fade_and_destroy(
    const int32_t  zsprite_id,
    const uint64_t duration_us)
{
    log_assert(duration_us > 0);
    
    // register scheduled animation
    T1TexQuadAnim * fade_destroy =
        T1_texquad_anim_request_next(true);
    fade_destroy->affect_zsprite_id = zsprite_id;
    fade_destroy->duration_us = duration_us;
    fade_destroy->gpu_vals.f32.rgba[3] = 0.0f;
    fade_destroy->del_obj_on_finish = true;
    fade_destroy->gpu_f32_active = true;
    T1_texquad_anim_commit(fade_destroy);
}

void T1_texquad_anim_fade_destroy_all(
    const uint64_t duration_us)
{
    T1_texquad_anim_fade_and_destroy(
        /* const int32_t  object_id: */
            T1_TEXQUAD_ID_HIT_EVERYTHING,
        /* const uint64_t duration_us: */
            duration_us);
}

void T1_texquad_anim_delete_all(void)
{
    tqas->mutex_lock(tqas->mutex_id);
    for (uint32_t i = 0; i < tqas->anims_size; i++)
    {
        tqas->anims[i].deleted = true;
    }
    tqas->mutex_unlock(tqas->mutex_id);
}

void T1_texquad_anim_resolve(void)
{
    tqas->mutex_lock(tqas->mutex_id);
    
    for (
        int32_t anim_i = (int32_t)tqas->anims_size - 1;
        anim_i >= 0;
        anim_i--)
    {
        T1InternalTexQuadAnim * anim =
            &tqas->anims[anim_i];
        
        T1_texquad_anim_resolve_single(anim);
    }
    
    tqas->mutex_unlock(tqas->mutex_id);
}

#elif T1_TEXQUAD_ANIM_ACTIVE == T1_INACTIVE
#else
#error
#endif // T1_SCHEDULED_ANIMS_ACTIVE
