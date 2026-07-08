#include "T1_texquad_anim.h"

#include <stddef.h>

#include "T1_mem.h"
#include "T1_global.h"
#include "T1_texquad.h"
#include "T1_log.h"

#if T1_TEXQUAD_ANIM_ACTIVE == T1_ACTIVE

typedef struct {
    T1TexQuadAnim public;
    
    /*
    If this flag is set, the target values represent endpoints, not deltas,
    until the animation is committed. During zsprite_anim_commit, the
    values will be converted to normal delta values, and the animation will
    be treated just like any other animation
    */
    u64 remaining_duration_us;
    u64 remaining_pause_us;
    f32 already_applied_t;
    
    b8 endpoints;
    
    b8 deleted;
    b8 committed;
} T1InternalTexQuadAnim;

#define FLT_TQDANIM_IGNORE 0xFFFF

typedef struct {
    T1InternalTexQuadAnim anims[T1_TEXQUAD_ANIMS_CAP];
    u32 (* init_mutex_and_return_id)(void);
    void (* mutex_lock)(const u32);
    void (* mutex_unlock)(const u32);
    u32 anims_size;
    u32 mutex_id;
} T1TexQuadAnimState;

static T1TexQuadAnimState * tqas = NULL;

void T1_texquad_anim_init(
    u32 (* funcptr_init_mutex_and_return_id)(void),
    void (* funcptr_mutex_lock)(u32),
    void (* funcptr_mutex_unlock)(u32))
{
    tqas = (T1TexQuadAnimState *)
        T1_mem_malloc_unmanaged(
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
        u32 i = 1;
        i < T1_TEXQUAD_ANIMS_CAP;
        i++)
    {
        T1_std_memcpy(
            &tqas->anims[i],
            &tqas->anims[0],
            sizeof(T1InternalTexQuadAnim));
        T1_log_assert(tqas->anims[i].deleted);
    }
    
    tqas->mutex_id = tqas->
        init_mutex_and_return_id();
}

static
T1InternalTexQuadAnim * T1_texquad_anim_get_container(
    const T1TexQuadAnim * public_ptr)
{
    T1_log_assert(
        offsetof(
            T1InternalTexQuadAnim,
            public) == 0);
    
    T1InternalTexQuadAnim * retval =
        (T1InternalTexQuadAnim *)
            public_ptr;
    
    T1_log_assert(
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
    T1_log_assert(
        !to_construct->committed);
    
    to_construct->public.
        target_T1_id = -1;
    to_construct->public.
        target_touch_id = -1;
    to_construct->public.runs = 1;
    
    T1_log_assert(
        !to_construct->deleted);
    T1_log_assert(
        !to_construct->committed);
}

T1TexQuadAnim * T1_texquad_anim_request_next(
    b8 endpoints_not_deltas)
{
    tqas->mutex_lock(tqas->mutex_id);
    
    T1_log_assert(
        tqas->anims_size <
            T1_TEXQUAD_ANIMS_CAP);
    T1InternalTexQuadAnim * return_value = NULL;
    
    for (
        u32 i = 0;
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
        T1_log_assert(tqas->anims_size < T1_TEXQUAD_ANIMS_CAP);
    }
    
    T1_log_assert(return_value->deleted);
    T1_texquad_anim_construct(
        return_value);
    
    T1_log_assert(!return_value->committed);
    T1_log_assert(!return_value->deleted);
    
    if (endpoints_not_deltas) {
        T1_std_memset_f32(
            &return_value->public.gpu_vals,
            T1_TEXQUADANIM_NO_EFFECT,
            sizeof(T1GPUTexQuad));
        return_value->endpoints = endpoints_not_deltas;
    }
    
    T1_log_assert(!return_value->committed);
    T1_log_assert(!return_value->deleted);
    
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
        u8 delete = anim->public.runs == 1;
        u8 reduce_runs = anim->public.runs > 0;
        
        if (delete) {
            anim->deleted = true;
            
            if (
                anim->public.
                    del_obj_on_finish)
            {
                if (
                    anim->public.target_T1_id ==
                    T1_TEXQUAD_ID_HIT_EVERYTHING)
                {
                    T1_texquad_delete_all();
                } else {
                    T1_texquad_delete(
                        anim->public.
                            target_T1_id);
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
    
    u64 elapsed = T1_global->elapsed;
    
    if (anim->remaining_pause_us > 0) {
        
        if (elapsed <= anim->remaining_pause_us) {
            anim->remaining_pause_us -= elapsed;
            return;
        } else {
            elapsed -= anim->remaining_pause_us;
            anim->remaining_pause_us = 0;
        }
    }
    
    T1_log_assert(anim->remaining_duration_us > 0);
    
    if (elapsed < anim->remaining_duration_us)
    {
        anim->remaining_duration_us -= elapsed;
    } else {
        anim->remaining_duration_us = 0;
    }
    
    u64 elapsed_so_far =
        anim->public.duration_us - anim->remaining_duration_us;
    T1_log_assert(
        elapsed_so_far <=
            anim->public.duration_us);
    
    f32 t_now = (f32)elapsed_so_far / (f32)anim->public.duration_us;
    T1_log_assert(t_now <= 1.0f);
    T1_log_assert(t_now >= 0.0f);
    T1_log_assert(t_now >= anim->already_applied_t);
    f32 t_applied = anim->already_applied_t;
    
    T1_log_assert(anim->already_applied_t <= t_now);
    anim->already_applied_t = t_now;
    
    t_now = T1_easing_t_to_eased_t(
        t_now,
        anim->public.easing_type);
    t_applied = T1_easing_t_to_eased_t(
        t_applied,
        anim->public.easing_type);
    
    if (anim->endpoints) {
        
        T1_texquad_apply_endpoint_anim(
            /* const s32 T1_id: */
                anim->public.
                    target_T1_id,
            /* const s32 touch_id: */
                anim->public.target_touch_id,
            /* const f32 t_applied: */
                t_applied,
            /* const f32 t_now: */
                t_now,
            /* const f32 * goal_gpu_vals_f32: */
                anim->public.gpu_f32_active ?
                    (f32 *)&anim->public.gpu_vals.f32s :
                    NULL,
            /* const s32 * goal_gpu_vals_s32: */
                anim->public.gpu_s32_active ?
                    (s32 *)&anim->public.gpu_vals.s32s :
                    NULL);
    } else {
        T1_texquad_apply_anim_effects_to_id(
            /* const s32 T1_id: */
                anim->public.target_T1_id,
            /* const s32 touch_id: */
                anim->public.target_touch_id,
            /* const f32 t_applied: */
                t_applied,
            /* const f32 t_now: */
                t_now,
            /* const f32 * anim_gpu_vals_f32: */
                anim->public.gpu_f32_active ?
                    (f32 *)&anim->
                        public.gpu_vals.f32s :
                    NULL,
            /* const s32 * anim_gpu_vals_s32: */
                anim->public.gpu_s32_active ?
                    (s32 *)&anim->
                        public.gpu_vals.s32s :
                    NULL);
    }
}

void T1_texquad_anim_commit(
    T1TexQuadAnim * to_commit)
{
    tqas->mutex_lock(tqas->mutex_id);
    
    T1InternalTexQuadAnim * parent =
        T1_texquad_anim_get_container(to_commit);
    T1_log_assert(&parent->public == to_commit);
    
    if (to_commit->del_conflict_anims)
    {
        for (
            u32 anim_i = 0;
            anim_i < tqas->anims_size;
            anim_i++)
        {
            T1InternalTexQuadAnim * a =
                &tqas->anims[anim_i];
            
            if (
                a->public.target_T1_id ==
                    to_commit->target_T1_id &&
                a->public.target_touch_id ==
                    to_commit->target_touch_id &&
                a->committed &&
                !a->public.del_obj_on_finish &&
                a->endpoints)
            {
                tqas->anims[anim_i].
                    deleted = true;
            }
        }
    }
    
    T1_log_assert(!parent->deleted);
    T1_log_assert(!parent->committed);
    
    if (to_commit->target_T1_id < 0) {
        T1_log_assert(to_commit->target_touch_id >= 0);
    } else {
        T1_log_assert(
            to_commit->target_touch_id == -1);
    }
    
    if (to_commit->target_touch_id < 0) {
        T1_log_assert(
            to_commit->target_T1_id >= 0);
    } else {
        T1_log_assert(
            to_commit->target_T1_id == -1);
    }
    
    T1_log_assert(parent->already_applied_t == 0.0f);
    
    parent->remaining_pause_us =
        to_commit->pause_us;
    parent->remaining_duration_us =
        to_commit->duration_us;
    
    T1_log_assert(parent->remaining_duration_us > 0);
    parent->committed = true;
    
    T1_log_assert(parent->committed);
    T1_log_assert(!parent->deleted);
    tqas->mutex_unlock(tqas->mutex_id);
}

void T1_texquad_anim_commit_and_instarun(
    T1TexQuadAnim * to_commit)
{
    tqas->mutex_lock(tqas->mutex_id);
    
    T1InternalTexQuadAnim * parent =
        T1_texquad_anim_get_container(to_commit);
    T1_log_assert(&parent->public == to_commit);
    T1_log_assert(to_commit->duration_us == 1);
    
    parent->remaining_pause_us =
        to_commit->pause_us;
    parent->remaining_duration_us =
        to_commit->duration_us;
    parent->committed = true;
    
    T1_texquad_anim_resolve_single(parent);
    parent->deleted = true;
    parent->committed = false;
    T1_log_assert(parent->remaining_duration_us == 0);
    
    tqas->mutex_unlock(tqas->mutex_id);
}

void
T1_texquad_anim_fade_to(
    const s32 T1_id,
    const u64 duration_us,
    const f32 target_alpha)
{
    T1_log_assert(T1_id >= 0);
    
    // register scheduled animation
    T1TexQuadAnim * modify_alpha = T1_texquad_anim_request_next(true);
    modify_alpha->target_T1_id = T1_id;
    modify_alpha->duration_us = duration_us < 1 ? 1 : duration_us;
    modify_alpha->gpu_vals.f32s.rgba[3] = target_alpha;
    modify_alpha->gpu_f32_active = true;
    if (modify_alpha->duration_us < 2) {
        T1_texquad_anim_commit_and_instarun(modify_alpha);
    } else {
        T1_texquad_anim_commit(modify_alpha);
    }    
}

void T1_texquad_anim_fade_and_destroy(
    const s32  T1_id,
    const u64 duration_us)
{
    T1_log_assert(duration_us > 0);
    
    // register scheduled animation
    T1TexQuadAnim * fade_destroy =
        T1_texquad_anim_request_next(true);
    fade_destroy->target_T1_id = T1_id;
    fade_destroy->duration_us = duration_us;
    fade_destroy->gpu_vals.f32s.rgba[3] = 0.0f;
    fade_destroy->del_obj_on_finish = true;
    fade_destroy->gpu_f32_active = true;
    T1_texquad_anim_commit(fade_destroy);
}

void T1_texquad_anim_fade_destroy_all(
    const u64 duration_us)
{
    T1_texquad_anim_fade_and_destroy(
        /* const s32 T1_id: */
            T1_TEXQUAD_ID_HIT_EVERYTHING,
        /* const u64 duration_us: */
            duration_us);
}

void T1_texquad_anim_delete_all(void)
{
    tqas->mutex_lock(tqas->mutex_id);
    for (u32 i = 0; i < tqas->anims_size; i++)
    {
        tqas->anims[i].deleted = true;
    }
    tqas->mutex_unlock(tqas->mutex_id);
}

void T1_texquad_anim_resolve(void)
{
    tqas->mutex_lock(tqas->mutex_id);
    
    for (
        s32 anim_i = (s32)tqas->anims_size - 1;
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
