#include "T1_anim.h"

#include <stddef.h>

#include "T1_mem.h"
#include "T1_log.h"
#include "T1_global.h"
#include "T1_types_cpu_to_gpu.h"
#include "T1_easing.h"
#include "T1_render_view.h"
#include "T1_zsprite.h"
#include "T1_texquad.h"
#include "T1_zlight.h"

#if T1_ANIM_ACTIVE == T1_ACTIVE

typedef struct {
    T1Anim public;
    
    /*
    If this flag is set, the target values represent
    endpoints, not deltas, until the animation is
    committed. During anim_commit, the values will be
    converted to normal delta values, and the
    animation will be treated just like any other
    animation
    */
    u64 remaining_duration_us;
    u64 remaining_pause_us;
    f32 already_applied_t;
    u16 zs_gpu_f32s_store_i;
    u16 zs_cpu_f32s_store_i;
    u16 zs_gpu_u32s_store_i;
    u16 tq_gpu_f32s_store_i;
    u16 tq_gpu_u32s_store_i;
    b8 endpoints_not_deltas;
    b8 deleted;
    b8 committed;
} T1AnimPrivate;

T1AnimPrivate * T1_anims = NULL;
u32 T1_anims_size = 0;

#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
static void T1_anim_sanity_check(T1AnimPrivate * a) {
    T1_log_assert(a->remaining_duration_us < 195000000);
    for (u32 i = 0; i < T1_anims_size; i++) {
        if (!a->endpoints_not_deltas) {
            if (a->public.zs_cpu_f32s) {
                T1_log_assert(
                    a->public.zs_cpu_f32s->bloom_on != T1_ANIM_NO_EFFECT);
            }
            T1_log_assert(a->public.zs_gpu_u32s == NULL);
        } else {
            if (a->public.zs_gpu_u32s) {
                f32 mix_rv_and_mix_tex_f32;
                T1_std_memcpy(
                    &mix_rv_and_mix_tex_f32,
                    &a->public.zs_gpu_u32s->mix_rv_and_mix_tex,
                    4);
                if (mix_rv_and_mix_tex_f32 != T1_ANIM_NO_EFFECT) {
                    s32 mix_rv = a->public.zs_gpu_u32s->mix_rv_and_mix_tex >> 16;
                    T1_log_assert(mix_rv < T1_RENDER_VIEW_CAP);
                    T1_log_assert(mix_rv >= -1);    
                }
                
                T1Tex mix_tex = a->public.zs_gpu_u32s->mix_rv_and_mix_tex & 0x0000FFFF;
                s16 mix_tex_array_i = T1_tex_to_array_i(mix_tex);
                s16 mix_tex_slice_i = T1_tex_to_slice_i(mix_tex);
                if (mix_tex != T1_TEX_NONE) {
                    T1_log_assert(mix_tex_array_i < T1_TEXARRAYS_CAP);
                    T1_log_assert(mix_tex_slice_i < T1_TEX_SLICES_CAP);
                }
                
                // T1_log_assert(a->public.zs_gpu_u32s->touch_id < 50000);
            }
        }
    }
    
    
}
#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif

typedef struct {
    T1GPUzSpritef32 zs_gpu_f32s_store[T1_ANIMS_CAP];
    T1GPUzSpriteu32 zs_gpu_u32s_store[T1_ANIMS_CAP];
    T1CPUzSpritef32 zs_cpu_f32s_store[T1_ANIMS_CAP];
    T1GPUTexQuadf32 tq_gpu_f32s_store[T1_ANIMS_CAP];
    T1GPUTexQuadu32 tq_gpu_u32s_store[T1_ANIMS_CAP];
    T1GPUzSprite final_vals;
    T1GPUzSprite deltas[2];
    T1CPUzSpritef32 zsprite_final_pos_cpu;
    u32 (* init_mutex_and_return_id)(void);
    void (* mutex_lock)(u32);
    void (* mutex_unlock)(u32);
    u32 mutex_id;
    b8 store_taken[T1_ANIMS_CAP];
} T1AnimState;

static T1AnimState * as = NULL;

void T1_anim_init(
    u32 (* funcptr_init_mutex_and_return_id)(void),
    void (* funcptr_mutex_lock)(u32),
    void (* funcptr_mutex_unlock)(u32))
{
    T1_anims = (T1AnimPrivate *)
        T1_mem_malloc_unmanaged(
            sizeof(T1AnimPrivate) * T1_ANIMS_CAP);
    T1_std_memset(
        T1_anims,
        0,
        sizeof(T1AnimPrivate) * T1_ANIMS_CAP);
    T1_anims[0].deleted = true;
    
    as = (T1AnimState *)T1_mem_malloc_unmanaged(
        sizeof(T1AnimState));
    T1_std_memset(
        as,
        0,
        sizeof(T1AnimState));
    
    as->init_mutex_and_return_id =
        funcptr_init_mutex_and_return_id;
    as->mutex_lock = funcptr_mutex_lock;
    as->mutex_unlock = funcptr_mutex_unlock;
    
    for (u32 i = 1; i < T1_ANIMS_CAP; i++) {
        T1_std_memcpy(
            &T1_anims[i],
            &T1_anims[0],
            sizeof(T1AnimPrivate));
        T1_log_assert(T1_anims[i].deleted);
    }
    
    as->mutex_id = as->init_mutex_and_return_id();
}

static
T1AnimPrivate *
T1_anim_get_container(
    const T1Anim * public_ptr)
{
    assert(
        offsetof(T1AnimPrivate, public) == 0);
    
    T1AnimPrivate * retval = (T1AnimPrivate *)public_ptr;
    
    T1_log_assert(
        &retval->public == public_ptr);
    
    return retval;
}

static void T1_anim_construct(
    T1AnimPrivate * to_construct)
{
    T1_std_memset(
        to_construct,
        0,
        sizeof(T1AnimPrivate));
    T1_log_assert(!to_construct->committed);
    
    to_construct->public.target_T1_id = T1_ID_NONE;
    to_construct->public.target_touch_id = T1_TOUCH_ID_NONE;
    to_construct->public.runs = 1;
    
    T1_log_assert(!to_construct->deleted);
    T1_log_assert(!to_construct->committed);
}

static T1GPUzSpritef32 *
T1_anim_fetch_next_store_zs_gpu_f32s(u16 * store_i) {
    u16 i = 0;
    while (
        ((as->store_taken[i] >> 4) & 1) &&
        i < T1_ANIMS_CAP)
    {
        i++;
    }
    T1_assert(i < T1_ANIMS_CAP);
    
    *store_i = i;
    as->store_taken[i] |= (1 << 4);
    T1GPUzSpritef32 * out = &as->zs_gpu_f32s_store[i];
    T1_std_memset(out, 0, sizeof(T1GPUzSpritef32));
    return out;
}

static T1CPUzSpritef32 *
T1_anim_fetch_next_store_zs_cpu_f32s(u16 * store_i) {  
    u16 i = 0;
    while (
        ((as->store_taken[i] >> 3) & 1) &&
        i < T1_ANIMS_CAP)
    {
        i++;
    }
    T1_assert(i < T1_ANIMS_CAP);
    
    *store_i = i;
    as->store_taken[i] |= (1 << 3);
    
    T1CPUzSpritef32 * out = &as->zs_cpu_f32s_store[i];
    
    T1_std_memset(out, 0, sizeof(T1CPUzSpritef32));
    return out;
}

static T1GPUzSpriteu32 *
T1_anim_fetch_next_store_zs_gpu_u32s(u16 * store_i) {
    u16 i = 0;
    while (
        ((as->store_taken[i] >> 2) & 1) &&
        i < T1_ANIMS_CAP)
    {
        i++;
    }
    T1_assert(i < T1_ANIMS_CAP);
    
    *store_i = i;
    as->store_taken[i] |= (1 << 2);
    
    T1GPUzSpriteu32 * out = &as->zs_gpu_u32s_store[i];
    
    T1_std_memset(out, 0, sizeof(T1GPUzSpriteu32));
    return out;
}

static T1GPUTexQuadf32 *
T1_anim_fetch_next_store_tq_gpu_f32s(u16 * store_i) {
    u16 i = 0;
    while (
        ((as->store_taken[i] >> 1) & 1) &&
        i < T1_ANIMS_CAP)
    {
        i++;
    }
    T1_assert(i < T1_ANIMS_CAP);
    
    *store_i = i;
    as->store_taken[i] |= (1 << 1);
    
    T1GPUTexQuadf32 * out = &as->tq_gpu_f32s_store[i];
    
    T1_std_memset(out, 0, sizeof(T1GPUTexQuadf32));
    return out;
}

static T1GPUTexQuadu32 *
T1_anim_fetch_next_store_tq_gpu_u32s(u16 * store_i) {
    u16 i = 0;
    while ((as->store_taken[i] & 1) && i < T1_ANIMS_CAP) {
        i++;
    }
    T1_assert(i < T1_ANIMS_CAP);
    
    *store_i = i;
    as->store_taken[i] |= 1;
    
    T1GPUTexQuadu32 * out = &as->tq_gpu_u32s_store[i];
    
    T1_std_memset(out, 0, sizeof(T1GPUTexQuadu32));
    return out;
}

T1Anim * T1_anim_request_next(
    b8 endpoints_not_deltas,
    b8 zs_gpu_f32s,
    b8 zs_cpu_f32s,
    b8 zs_gpu_u32s,
    b8 tq_gpu_f32s,
    b8 tq_gpu_u32s)
{
    as->mutex_lock(as->mutex_id);
    
    T1_log_assert(T1_anims_size < T1_ANIMS_CAP);
    T1AnimPrivate * out = NULL;
    
    for (
        u32 i = 0;
        i < T1_anims_size;
        i++)
    {
        if (T1_anims[i].deleted)
        {
            out = &T1_anims[i];
            break;
        }
    }
    
    if (out == NULL) {
        out =
            &T1_anims[T1_anims_size];
        T1_anims_size += 1;
        T1_log_assert(T1_anims_size < T1_ANIMS_CAP);
    }
    
    T1_log_assert(out->deleted);
    T1_anim_construct(out);
    
    T1_log_assert(!out->committed);
    T1_log_assert(!out->deleted);
    
    if (zs_gpu_f32s) {
        out->public.zs_gpu_f32s =
            T1_anim_fetch_next_store_zs_gpu_f32s(
                &out->zs_gpu_f32s_store_i);
    }
    
    if (zs_cpu_f32s) {
        out->public.zs_cpu_f32s =
            T1_anim_fetch_next_store_zs_cpu_f32s(
                &out->zs_cpu_f32s_store_i);
    }
    
    if (zs_gpu_u32s) {
        out->public.zs_gpu_u32s =
            T1_anim_fetch_next_store_zs_gpu_u32s(
                &out->zs_gpu_u32s_store_i);
    }
    
    if (tq_gpu_f32s) {
        out->public.tq_gpu_f32s =
            T1_anim_fetch_next_store_tq_gpu_f32s(
                &out->tq_gpu_f32s_store_i);
    }
    
    if (tq_gpu_u32s) {
        out->public.tq_gpu_u32s =
            T1_anim_fetch_next_store_tq_gpu_u32s(
                &out->tq_gpu_u32s_store_i);
    }
    
    if (endpoints_not_deltas) {
        //        f32 bloom_on_b4 = out->public.zs_cpu_f32s ?
        //            out->public.zs_cpu_f32s->bloom_on :
        //            -12345.0f;
        if (out->public.zs_gpu_f32s) {
            T1_std_memset_f32(
                out->public.zs_gpu_f32s,
                T1_ANIM_NO_EFFECT,
                sizeof(T1GPUzSpritef32));
        }
        //        f32 bloom_on_now = out->public.zs_cpu_f32s ?
        //            out->public.zs_cpu_f32s->bloom_on :
        //            -12345.0f;
        // T1_log_assert(bloom_on_now == bloom_on_b4);
        if (out->public.zs_gpu_u32s) {
            T1_std_memset_f32(
                out->public.zs_gpu_u32s,
                T1_ANIM_NO_EFFECT,
                sizeof(T1GPUzSpriteu32));
        }
        // bloom_on_now = out->public.zs_cpu_f32s ?
        //     out->public.zs_cpu_f32s->bloom_on :
        //     -12345.0f;
        // T1_log_assert(bloom_on_now == bloom_on_b4);
        if (out->public.zs_cpu_f32s) {
            T1_std_memset_f32(
                out->public.zs_cpu_f32s,
                T1_ANIM_NO_EFFECT,
                sizeof(T1CPUzSpritef32));    
        }
        //bloom_on_b4 = out->public.zs_cpu_f32s ?
        //    out->public.zs_cpu_f32s->bloom_on :
        //    -12345.0f;
        if (out->public.tq_gpu_f32s) {
            T1_std_memset_f32(
                out->public.tq_gpu_f32s,
                T1_ANIM_NO_EFFECT,
                sizeof(T1GPUTexQuadf32));
        }
        // bloom_on_now = out->public.zs_cpu_f32s ?
        //     out->public.zs_cpu_f32s->bloom_on :
        //     -12345.0f;
        // T1_log_assert(bloom_on_now == bloom_on_b4);
        if (out->public.tq_gpu_u32s) {
            T1_std_memset_f32(
                out->public.tq_gpu_u32s,
                T1_ANIM_NO_EFFECT,
                sizeof(T1GPUTexQuadu32));
        }
        //        bloom_on_now = out->public.zs_cpu_f32s ?
        //            out->public.zs_cpu_f32s->bloom_on :
        //            -12345.0f;
        // T1_log_assert(bloom_on_now == bloom_on_b4);
        out->endpoints_not_deltas = endpoints_not_deltas;
    }
    
    T1_log_assert(!out->committed);
    T1_log_assert(!out->deleted);
    
    as->mutex_unlock(as->mutex_id);
    
    return &out->public;
}

static void T1_anim_delete(T1AnimPrivate * a) {
    if (a->public.zs_gpu_f32s) {
        as->store_taken[a->zs_gpu_f32s_store_i] &= ~(1 << 4);
    }
    if (a->public.zs_cpu_f32s) {
        as->store_taken[a->zs_cpu_f32s_store_i] &= ~(1 << 3);
    }
    if (a->public.zs_gpu_u32s) {
        as->store_taken[a->zs_gpu_u32s_store_i] &= ~(1 << 2);
    }
    if (a->public.tq_gpu_f32s) {
        as->store_taken[a->tq_gpu_f32s_store_i] &= ~(1 << 1);
    }
    if (a->public.tq_gpu_u32s) {
        as->store_taken[a->tq_gpu_u32s_store_i] &= ~(1);
    }
    
    a->deleted = true;
    a->committed = false;
    
    if (a == (T1_anims + (s32)T1_anims_size-1)) { T1_anims_size -= 1; }
}

static void T1_anim_resolve_single(
    T1AnimPrivate * a)
{
    if (a->deleted || !a->committed) {
        return;
    }
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    T1_anim_sanity_check(a);
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    if (a->already_applied_t >= 1.0f) {
        u8 delete = a->public.runs == 1;
        u8 reduce_runs = a->public.runs > 0;
        
        if (delete) {
            T1_anim_delete(a);
            
            if (a->public.del_obj_on_finish)
            {
                if (
                    a->public.target_T1_id ==
                    T1_ANIM_HIT_EVERYTHING)
                {
                    T1_zsprite_delete_all();
                    T1_texquad_delete_all();
                    T1_zlight_delete_all();
                } else {
                    T1_zsprite_delete(
                        a->public.target_T1_id);
                    T1_texquad_delete(
                        a->public.target_T1_id);
                }
            }
        } else {
            a->remaining_duration_us =
                a->public.duration_us;
            a->remaining_pause_us =
                a->public.pause_us;
        }
        
        if (reduce_runs) {
            a->public.runs -= 1;
        }
        
        a->already_applied_t = 0.0f;
        
        return;
    }
    
    u64 elapsed = T1_global->elapsed;
    
    if (a->remaining_pause_us > 0) {
        if (elapsed <= a->remaining_pause_us) {
            a->remaining_pause_us -= elapsed;
            return;
        } else {
            elapsed -= a->remaining_pause_us;
            a->remaining_pause_us = 0;
        }
    }
    
    T1_log_assert(a->remaining_duration_us > 0);
    
    if (elapsed < a->remaining_duration_us) {
        a->remaining_duration_us -= elapsed;
    } else {
        a->remaining_duration_us = 0;
    }
    
    u64 elapsed_so_far = a->public.duration_us -
        a->remaining_duration_us;
    T1_log_assert(elapsed_so_far <=
        a->public.duration_us);
    
    f32 t_now = (f32)elapsed_so_far / (f32)a->public.duration_us;
    T1_log_assert(t_now <= 1.0f);
    T1_log_assert(t_now >= 0.0f);
    T1_log_assert(t_now >= a->already_applied_t);
    f32 t_applied = a->already_applied_t;
    
    T1_log_assert(a->already_applied_t <= t_now);
    a->already_applied_t = t_now;
    
    t_now = T1_easing_t_to_eased_t(
        t_now,
        a->public.easing_type);
    t_applied = T1_easing_t_to_eased_t(
        t_applied,
        a->public.easing_type);
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    T1_anim_sanity_check(a);
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    if (a->endpoints_not_deltas) {
        if (
            a->public.zs_gpu_f32s ||
            a->public.zs_gpu_u32s ||
            a->public.zs_cpu_f32s)
        {
            T1_zsprite_apply_endpoint_anim(
                /* u32 T1_id: */
                    a->public.target_T1_id,
                /* u32 touch_id: */
                    a->public.target_touch_id,
                /* f32 t_applied: */
                    t_applied,
                /* f32 t_now: */
                    t_now,
                /* f32 * goal_gpu_vals_f32: */
                    (f32 *)a->public.zs_gpu_f32s,
                /* const u32 * goal_gpu_vals_u32: */
                    (u32 *)a->public.zs_gpu_u32s,
                /* const f32 * goal_cpu_vals: */
                    (f32 *)a->public.zs_cpu_f32s);
        }
        if (
            a->public.tq_gpu_f32s ||
            a->public.tq_gpu_u32s)
        {
            T1_texquad_apply_endpoint_anim(
                /* u32 T1_id: */
                    a->public.target_T1_id,
                /* u32 touch_id: */
                    a->public.target_touch_id,
                /* f32 t_applied: */
                    t_applied,
                /* f32 t_now: */
                    t_now,
                /* const f32 * goal_gpu_vals_f32: */
                    (f32 *)a->public.tq_gpu_f32s,
                /* const u32 * goal_gpu_vals_u32: */
                    (u32 *)a->public.tq_gpu_u32s);
        }
    } else {
        if (
            a->public.zs_gpu_f32s ||
            a->public.zs_gpu_u32s ||
            a->public.zs_cpu_f32s)
        {
            T1_zsprite_apply_anim_effects_to_id(
                /* u32 T1_id: */
                    a->public.target_T1_id,
                /* u32 touch_id: */
                    a->public.target_touch_id,
                /* f32 t_applied: */
                    t_applied,
                /* f32 t_now: */
                    t_now,
                /* const f32 * anim_gpu_f32s: */
                    (f32 *)a->public.zs_gpu_f32s,
                /* const u32 * anim_gpu_u32s: */
                    (u32 *)a->public.zs_gpu_u32s,
                /* const f32 * anim_cpu_f32s: */
                    (f32 *)a->public.zs_cpu_f32s);
        }
        if (
            a->public.tq_gpu_f32s ||
            a->public.tq_gpu_u32s)
        {
            T1_texquad_apply_anim_effects_to_id(
                /* u32 T1_id: */
                    a->public.target_T1_id,
                /* u32 touch_id: */
                    a->public.target_touch_id,
                /* f32 t_applied: */
                    t_applied,
                /* f32 t_now: */
                    t_now,
                /* const f32 * anim_gpu_vals_f32: */
                    (f32 *)a->public.tq_gpu_f32s,
                /* const u32 * anim_gpu_vals_u32: */
                    (u32 *)a->public.tq_gpu_u32s);
        }
    }
}

void T1_anim_commit_and_instarun(
    T1Anim * to_commit
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    ,const char * original_func_name
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    )
{
    as->mutex_lock(as->mutex_id);
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    T1_std_strcat_cap(
        to_commit->original_func_name,
        64,
        original_func_name);
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_anim_assert_anim_valid_before_commit(to_commit);
    
    T1AnimPrivate * parent =
        T1_anim_get_container(to_commit);
    T1_log_assert(&parent->public == to_commit);
    T1_log_assert(to_commit->duration_us == 1);
    
    parent->remaining_pause_us = to_commit->pause_us;
    parent->remaining_duration_us = to_commit->duration_us;
    parent->committed = true;
    
    T1_anim_sanity_check(parent);
    
    T1_anim_resolve_single(parent);
    T1_anim_delete(parent);
    parent->committed = false;
    T1_log_assert(parent->remaining_duration_us == 0);
    
    as->mutex_unlock(as->mutex_id);
}

void T1_anim_assert_anim_valid_before_commit(
    T1Anim * to_check) {
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    T1AnimPrivate * parent = T1_anim_get_container(to_check);
    
    T1_log_assert(to_check->original_func_name[0] != '\0');
    T1_log_assert(&parent->public == to_check);
    
    if (!parent->endpoints_not_deltas) {
        // not an endpoint anim, regular deltas
        
        // signed ints don't play nice without endpoints
        T1_log_assert(!parent->public.zs_gpu_u32s);
        
        if (parent->public.zs_gpu_f32s) {
            // don't reduce alpha by 1 to fade out, use
            // an endpoint anim
            T1_log_assert(parent->public.zs_gpu_f32s->alpha > -0.9f);
        }
    }
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_log_assert(to_check->duration_us > 0);
    
    T1_anim_sanity_check(parent);
}

void T1_anim_commit(
    T1Anim * c
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    ,const char * original_func_name
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    )
{
    as->mutex_lock(as->mutex_id);
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    T1_std_strcat_cap(
        c->original_func_name,
        128,
        original_func_name);
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_anim_assert_anim_valid_before_commit(c);
    
    T1AnimPrivate * parent = T1_anim_get_container(c);
    T1_log_assert(&parent->public == c);
    
    if (c->del_conflict_anims)
    {
        for (
            s32 anim_i = (s32)T1_anims_size - 1;
            anim_i >= 0;
            anim_i--)
        {
            T1AnimPrivate * a = &T1_anims[anim_i];
            
            if (
                (void *)a != (void *)c &&
                (a->public.target_T1_id ==
                    c->target_T1_id ||
                a->public.target_T1_id ==
                    T1_ANIM_HIT_EVERYTHING) &&
                a->public.target_touch_id ==
                    c->target_touch_id &&
                a->committed &&
                a->endpoints_not_deltas)
            {
                T1_anim_delete(T1_anims + anim_i);
            }
        }
    }
    
    if (!parent->endpoints_not_deltas) {
        // don't use T1_ANIM_NO_EFFECT in non-endpoint anims
        if (parent->public.zs_cpu_f32s) {
            for (u32 i = 0; i < (sizeof(T1CPUzSpritef32) / 4); i++) {
                T1_assert(((f32 *)parent->public.zs_cpu_f32s)[i] != T1_ANIM_NO_EFFECT);
                T1_assert(((f32 *)parent->public.zs_cpu_f32s)[i] != 65535.0f);
            }
        }
        if (parent->public.zs_gpu_f32s) {
            for (u32 i = 0; i < (sizeof(T1GPUzSpritef32) / 4); i++) {
                T1_assert(((f32 *)parent->public.zs_gpu_f32s)[i] != T1_ANIM_NO_EFFECT);
                T1_assert(((f32 *)parent->public.zs_gpu_f32s)[i] != 65535.0f);
            }
        }
        if (parent->public.zs_gpu_u32s) {
            for (u32 i = 0; i < (sizeof(T1GPUzSpriteu32) / 4); i++) {
                f32 check;
                T1_std_memcpy(&check, ((u32 *)parent->public.zs_gpu_u32s) + i, 4);
                T1_assert(check != T1_ANIM_NO_EFFECT);
                T1_assert(check != 65535.0f);
            }
        }
    }
    
    T1_log_assert(!parent->deleted);
    T1_log_assert(!parent->committed);
    
    if (c->target_T1_id == T1_ID_NONE) {
        T1_log_assert(c->target_touch_id != T1_TOUCH_ID_NONE);
    } else {
        T1_log_assert(c->target_touch_id == T1_TOUCH_ID_NONE);
    }
    
    if (c->target_touch_id == T1_TOUCH_ID_NONE) {
        T1_log_assert(c->target_T1_id >= 0);
    } else {
        T1_log_assert(c->target_T1_id == T1_ID_NONE);
    }
    
    T1_log_assert(parent->already_applied_t == 0.0f);
    
    parent->remaining_pause_us = c->pause_us;
    parent->remaining_duration_us = c->duration_us;
    
    T1_log_assert(parent->remaining_duration_us > 0);
    parent->committed = true;
    
    T1_log_assert(parent->committed);
    T1_log_assert(!parent->deleted);
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    T1_anim_sanity_check(parent);
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    as->mutex_unlock(as->mutex_id);
}

void T1_anim_shatter_and_destroy(
    u32 T1_id,
    u64 duration_us) {
    #if T1_LOGGER_ASSERTS_ACTIVE
    #else
    (void)duration_us;
    #endif
    
    T1_log_assert(duration_us > 0);
    T1_log_assert(T1_id >= 0);
    
    T1Anim * set_scatter_mesh =
    T1_anim_request_next(
        /* b8 endpoints_not_deltas: */ true,
        /* b8 zs_gpu_f32s: */ false,
        /* b8 zs_cpu_f32s: */ true,
        /* b8 zs_gpu_u32s: */ false,
        /* b8 tq_gpu_f32s: */ false,
        /* b8 tq_gpu_u32s: */ false);
    set_scatter_mesh->target_T1_id = T1_id;
    set_scatter_mesh->zs_cpu_f32s->alpha_on = 1.0f;
    set_scatter_mesh->duration_us = 1;
    T1_anim_commit_and_instarun(
        set_scatter_mesh
        #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
        , "T1_anim_shatter_and_destroy"
        #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        );
    
    T1Anim * scatter = T1_anim_request_next(
        true,
        /* b8 zs_gpu_f32s: */ true,
        /* b8 zs_cpu_f32s: */ false,
        /* b8 zs_gpu_u32s: */ false,
        /* b8 tq_gpu_f32s: */ false,
        /* b8 tq_gpu_u32s: */ false);
    scatter->zs_gpu_f32s->alpha = 0.0f;
    scatter->target_T1_id = T1_id;
    scatter->duration_us = duration_us;
    scatter->easing_type = T1_EASINGTYPE_NONE;
    scatter->runs = 1;
    scatter->del_obj_on_finish = true;
    T1_anim_commit(
        scatter
        #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
        , "T1_anim_shatter_and_destroy"
        #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        );
}

void T1_anim_evaporate_and_destroy(
    u32 T1_id,
    u64 duration_us) {
    #if T1_LOGGER_ASSERTS_ACTIVE
    #else
    (void)duration_us;
    #endif
    
    T1_log_assert(duration_us > 0);
    T1_log_assert(T1_id >= 0);
    
    T1Anim * set_scatter_mesh =
    T1_anim_request_next(
        /* b8 endpoints_not_deltas: */ true,
        /* b8 zs_gpu_f32s: */ false,
        /* b8 zs_cpu_f32s: */ true,
        /* b8 zs_gpu_u32s: */ false,
        /* b8 tq_gpu_f32s: */ false,
        /* b8 tq_gpu_u32s: */ false);
    set_scatter_mesh->target_T1_id = T1_id;
    set_scatter_mesh->zs_cpu_f32s->alpha_on = 1.0f;
    set_scatter_mesh->duration_us = 1;
    T1_anim_commit_and_instarun(
        set_scatter_mesh
        #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
        , "T1_anim_evaporate_and_destroy"
        #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        );
    
    T1Anim * evap = T1_anim_request_next(
        /* b8 endpoints_not_deltas: */ true,
        /* b8 zs_gpu_f32s: */ true,
        /* b8 zs_cpu_f32s: */ false,
        /* b8 zs_gpu_u32s: */ false,
        /* b8 tq_gpu_f32s: */ false,
        /* b8 tq_gpu_u32s: */ false);
    evap->zs_gpu_f32s->alpha = 0.0f;
    evap->target_T1_id = T1_id;
    evap->duration_us = duration_us;
    evap->easing_type = T1_EASINGTYPE_NONE;
    evap->runs = 1;
    evap->del_obj_on_finish = true;
    T1_anim_commit(
        evap
        #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
        , "T1_anim_evaporate_and_destroy"
        #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        );
}

void T1_anim_fade_and_destroy(
    u32 T1_id,
    u64 duration_us) {
    T1_log_assert(duration_us > 0);
    
    // register scheduled animation
    T1Anim * fade_destroy = T1_anim_request_next(
        /* b8 endpoints_not_deltas: */  true,
        /* b8 zs_gpu_f32s: */ true,
        /* b8 zs_cpu_f32s: */ false,
        /* b8 zs_gpu_u32s: */ false,
        /* b8 tq_gpu_f32s: */ false,
        /* b8 tq_gpu_u32s: */ false);
    fade_destroy->target_T1_id = T1_id;
    fade_destroy->duration_us = duration_us;
    fade_destroy->zs_gpu_f32s->alpha = 0.0f;
    fade_destroy->zs_gpu_f32s->shadow_strength = 0.0f;
    fade_destroy->del_obj_on_finish = true;
    T1_anim_commit(
        fade_destroy
        #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
        , "T1_anim_fade_and_destroy"
        #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        );
}

void T1_anim_fade_destroy_all(
    u64 duration_us) {
    T1_anim_fade_and_destroy(
        /* u32 T1_id: */
            T1_ANIM_HIT_EVERYTHING,
        /* u64 duration_us: */
            duration_us);
}

void T1_anim_fade_to(
    u32 T1_id,
    u64 duration_us,
    f32 target_alpha) {
    T1_log_assert(T1_id >= 0);
    
    // register scheduled animation
    T1Anim * modify_alpha = T1_anim_request_next(
        /* b8 endpoints_not_deltas: */ true,
        /* b8 zs_gpu_f32s: */ true,
        /* b8 zs_cpu_f32s: */ false,
        /* b8 zs_gpu_u32s: */ false,
        /* b8 tq_gpu_f32s: */ false,
        /* b8 tq_gpu_u32s: */ false);
    modify_alpha->target_T1_id = T1_id;
    modify_alpha->duration_us = duration_us;
    modify_alpha->zs_gpu_f32s->alpha = target_alpha;
    modify_alpha->del_obj_on_finish = true;
    if (duration_us < 50) {
        T1_anim_commit_and_instarun(
            modify_alpha
            #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
            , "T1_anim_fade_to"
            #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
            #else
            #error
            #endif
            );
    } else {
        T1_anim_commit(
            modify_alpha
            #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
            , "T1_anim_fade_to"
            #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
            #else
            #error
            #endif
            );
    }
}

void T1_anim_resolve(void) {
    as->mutex_lock(as->mutex_id);
    
    for (
        s32 anim_i = (s32)T1_anims_size - 1;
        anim_i >= 0;
        anim_i--)
    {
        T1AnimPrivate * anim = &T1_anims[anim_i];
        
        T1_anim_resolve_single(anim);
    }
    
    as->mutex_unlock(as->mutex_id);
}

void T1_anim_dud_dance(
    u32 T1_id,
    f32 magnitude)
{
    T1Anim * move_request =
    T1_anim_request_next(
        /* b8 endpoints_not_deltas: */ false,
        /* b8 zs_gpu_f32s: */ false,
        /* b8 zs_cpu_f32s: */ true,
        /* b8 zs_gpu_u32s: */ false,
        /* b8 tq_gpu_f32s: */ false,
        /* b8 tq_gpu_u32s: */ false);
    move_request->easing_type = T1_EASINGTYPE_QUADRUPLE_BOUNCE_ZERO_TO_ZERO;
    move_request->target_T1_id = T1_id;
    move_request->zs_cpu_f32s->xyz[0] = magnitude * 0.05f;
    move_request->zs_cpu_f32s->xyz[1] = magnitude * 0.035f;
    move_request->zs_cpu_f32s->xyz[2] = magnitude * 0.005f;
    move_request->duration_us = 300000;
    T1_anim_commit(
        move_request
        #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
        , "T1_anim_dud_dance"
        #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        );
}

void T1_anim_bump(
    u32 T1_id,
    u32 wait)
{
    #if T1_LOGGER_ASSERTS_ACTIVE
    T1_log_assert(wait == 0.0f);
    #else
    (void)wait;
    #endif
    
    T1Anim * bump_req = T1_anim_request_next(
        /* b8 endpoints_not_deltas: */ false,
        /* b8 zs_gpu_f32s: */ false,
        /* b8 zs_cpu_f32s: */ true,
        /* b8 zs_gpu_u32s: */ false,
        /* b8 tq_gpu_f32s: */ false,
        /* b8 tq_gpu_u32s: */ false);
    bump_req->easing_type =
        T1_EASINGTYPE_DOUBLE_BOUNCE_ZERO_TO_ZERO;
    bump_req->target_T1_id = T1_id;
    bump_req->zs_cpu_f32s->mul_xyz[0] = 0.05f;
    bump_req->zs_cpu_f32s->mul_xyz[1] = 0.05f;
    bump_req->zs_cpu_f32s->mul_xyz[2] = 0.05f;
    bump_req->duration_us = 200000;
    T1_anim_commit(
        bump_req
        #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
        , "T1_anim_bump"
        #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        );
}

void T1_anim_delete_all(void) {
    as->mutex_lock(as->mutex_id);
    for (u32 i = 0; i < T1_anims_size; i++) {
        T1_anim_delete(T1_anims + i);
    }
    as->mutex_unlock(as->mutex_id);
}

#if 0
void T1_anim_delete_endpoint_anims_targeting(
    u32 T1_id) {
    as->mutex_lock(as->mutex_id);
    for (u32 i = 0; i < T1_anims_size; i++) {
        if (
            !T1_anims[i].deleted &&
            T1_anims[i].endpoints_not_deltas &&
            (T1_anims[i].public.
                target_T1_id == T1_id ||
            T1_anims[i].
                public.target_T1_id ==
                    T1_ZSPRITE_ID_HIT_EVERYTHING))
        {
            T1_anim_delete(T1_anims + i);
        }
    }
    as->mutex_unlock(as->mutex_id);
}
#endif

void T1_anim_delete_all_anims_targeting(u32 T1_id) {
    as->mutex_lock(as->mutex_id);
    for (u32 i = 0; i < T1_anims_size; i++) {
        T1Anim * a = &T1_anims[i].public;
        
        if (
            !T1_anims[i].deleted &&
            T1_anims[i].committed &&
            (a->target_T1_id == T1_id ||
            a->target_T1_id == T1_ANIM_HIT_EVERYTHING))
        {
            T1_anim_delete(T1_anims + i);
        }
    }
    as->mutex_unlock(as->mutex_id);
}
#elif T1_ANIM_ACTIVE == T1_INACTIVE
#else
#error
#endif // T1_ZSPRITE_ANIM_ACTIVE

