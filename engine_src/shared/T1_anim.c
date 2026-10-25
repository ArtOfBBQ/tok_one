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

/*
The client only sees anim handles (indices into T1_anims)

The stores hold the values to animate to (endpoints_not_deltas) or by.
They're fetched when the first property for them is set, unused values
in an endpoint store are T1_ANIM_NO_EFFECT, in a delta store 0
*/
typedef struct {
    T1GPUzSpritef32 * zs_gpu_f32s;
    T1CPUzSpritef32 * zs_cpu_f32s;
    T1GPUzSpriteu32 * zs_gpu_u32s;
    T1GPUTexQuadf32 * tq_gpu_f32s;
    T1GPUTexQuadu32 * tq_gpu_u32s;
    T1zLightf32     * zl_gpu_f32s;
    
    void (* run_func_on_finish)(void);
    u64 duration_us;
    u64 pause_us;
    u64 remaining_duration_us;
    u64 remaining_pause_us;
    f32 already_applied_t;
    u16 zs_gpu_f32s_store_i;
    u16 zs_cpu_f32s_store_i;
    u16 zs_gpu_u32s_store_i;
    u16 tq_gpu_f32s_store_i;
    u16 tq_gpu_u32s_store_i;
    u16 zl_f32s_store_i;
    u32 runs;
    u32 target_T1_id;
    T1EasingType easing_type; // u8
    b8 del_obj_on_finish;
    b8 del_conflict_anims;
    b8 endpoints_not_deltas;
    b8 deleted;
    b8 committed;
} T1Anim;

T1Anim * T1_anims = NULL;
u32 T1_anims_size = 0;


#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
static void T1_anim_sanity_check(T1Anim * a) {
    T1_log_assert(a->remaining_duration_us < 195000000);
    
    if (!a->endpoints_not_deltas) {
        if (a->zs_cpu_f32s) {
            T1_log_assert(a->zs_cpu_f32s->bloom_on != T1_ANIM_NO_EFFECT);
        }
        T1_log_assert(a->zs_gpu_u32s == NULL);
    } else if (a->zs_gpu_u32s) {
        f32 mix_rv_and_mix_tex_f32;
        T1_std_memcpy(
            &mix_rv_and_mix_tex_f32,
            &a->zs_gpu_u32s->mix_rv_and_mix_tex,
            4);
        if (mix_rv_and_mix_tex_f32 != T1_ANIM_NO_EFFECT) {
            // like the shader: rv is the low 16 bits, tex the high 16
            s16 mix_rv = (s16)(a->zs_gpu_u32s->mix_rv_and_mix_tex & 0xFFFF);
            T1_log_assert(mix_rv < T1_RENDER_VIEW_CAP);
            T1_log_assert(mix_rv >= -1);
            
            T1Tex mix_tex = (T1Tex)(a->zs_gpu_u32s->mix_rv_and_mix_tex >> 16);
            if (mix_tex != T1_TEX_NONE) {
                T1_log_assert(T1_tex_to_array_i(mix_tex) < T1_TEXARRAYS_CAP);
                T1_log_assert(T1_tex_to_slice_i(mix_tex) < T1_TEX_SLICES_CAP);
            }
        }
    }
}
#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#define T1_anim_sanity_check(x)
#else
#error
#endif

typedef struct {
    T1zLightf32     zl_gpu_f32s_store[T1_ANIMS_CAP];
    T1GPUzSpritef32 zs_gpu_f32s_store[T1_ANIMS_CAP];
    T1GPUzSpriteu32 zs_gpu_u32s_store[T1_ANIMS_CAP];
    T1CPUzSpritef32 zs_cpu_f32s_store[T1_ANIMS_CAP];
    T1GPUTexQuadf32 tq_gpu_f32s_store[T1_ANIMS_CAP];
    T1GPUTexQuadu32 tq_gpu_u32s_store[T1_ANIMS_CAP];
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
    T1_anims = (T1Anim *)
        T1_mem_malloc_unmanaged(
            sizeof(T1Anim) * T1_ANIMS_CAP);
    T1_std_memset(
        T1_anims,
        0,
        sizeof(T1Anim) * T1_ANIMS_CAP);
    T1_anims[0].deleted = 1;
    
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
            sizeof(T1Anim));
        T1_log_assert(T1_anims[i].deleted);
    }
    
    as->mutex_id = as->init_mutex_and_return_id();
}

static void T1_anim_construct(
    T1Anim * to_construct)
{
    T1_std_memset(
        to_construct,
        0,
        sizeof(T1Anim));
    T1_log_assert(!to_construct->committed);
    
    to_construct->target_T1_id = T1_ID_NONE;
    to_construct->runs = 1;
    
    T1_log_assert(!to_construct->deleted);
    T1_log_assert(!to_construct->committed);
}

static T1zLightf32 * T1_anim_fetch_next_store_zl_gpu_f32s(
    u16 * store_i)
{
    u16 i = 0;
    while (
        ((as->store_taken[i] >> 5) & 1) &&
        i < T1_ANIMS_CAP)
    {
        i++;
    }
    T1_log_assert(i < T1_ANIMS_CAP);
    
    *store_i = i;
    as->store_taken[i] |= (1 << 5);
    T1zLightf32 * out = &as->zl_gpu_f32s_store[i];
    T1_std_memset(out, 0, sizeof(T1zLightf32));
    return out;
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
    T1_log_assert(i < T1_ANIMS_CAP);
    
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
    T1_log_assert(i < T1_ANIMS_CAP);
    
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
    T1_log_assert(i < T1_ANIMS_CAP);
    
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
    T1_log_assert(i < T1_ANIMS_CAP);
    
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
    T1_log_assert(i < T1_ANIMS_CAP);
    
    *store_i = i;
    as->store_taken[i] |= 1;
    
    T1GPUTexQuadu32 * out = &as->tq_gpu_u32s_store[i];
    
    T1_std_memset(out, 0, sizeof(T1GPUTexQuadu32));
    return out;
}

static T1Anim * T1_anim_alloc(b8 endpoints_not_deltas)
{
    as->mutex_lock(as->mutex_id);
    
    T1_log_assert(T1_anims_size < T1_ANIMS_CAP);
    T1Anim * out = NULL;
    
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
    out->endpoints_not_deltas = endpoints_not_deltas;
    
    T1_log_assert(!out->committed);
    T1_log_assert(!out->deleted);
    
    as->mutex_unlock(as->mutex_id);
    
    return out;
}

static void T1_anim_delete(T1Anim * a) {
    if (a->zl_gpu_f32s) {
        as->store_taken[a->zl_f32s_store_i] &= ~(1 << 5);
    }
    if (a->zs_gpu_f32s) {
        as->store_taken[a->zs_gpu_f32s_store_i] &= ~(1 << 4);
    }
    if (a->zs_cpu_f32s) {
        as->store_taken[a->zs_cpu_f32s_store_i] &= ~(1 << 3);
    }
    if (a->zs_gpu_u32s) {
        as->store_taken[a->zs_gpu_u32s_store_i] &= ~(1 << 2);
    }
    if (a->tq_gpu_f32s) {
        as->store_taken[a->tq_gpu_f32s_store_i] &= ~(1 << 1);
    }
    if (a->tq_gpu_u32s) {
        as->store_taken[a->tq_gpu_u32s_store_i] &= ~(1);
    }
    
    a->deleted = 1;
    a->committed = 0;
    
    if (a == (T1_anims + (s32)T1_anims_size-1)) { T1_anims_size -= 1; }
}

static void T1_anim_resolve_single(
    T1Anim * a)
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
        u8 delete = a->runs == 1;
        u8 reduce_runs = a->runs > 0;
        
        if (delete) {
            T1_anim_delete(a);
            
            if (a->run_func_on_finish) {
                a->run_func_on_finish();
            }
            
            if (a->del_obj_on_finish)
            {
                if (
                    a->target_T1_id ==
                        T1_ANIM_HIT_EVERYTHING)
                {
                    T1_zsprite_delete_all();
                    T1_texquad_delete_all();
                    T1_zlight_delete_all();
                } else {
                    T1_zsprite_delete(a->target_T1_id);
                    T1_texquad_delete(a->target_T1_id);
                    T1_zlight_delete(a->target_T1_id);
                }
            }
        } else {
            a->remaining_duration_us = a->duration_us;
            a->remaining_pause_us = a->pause_us;
        }
        
        if (reduce_runs) {
            a->runs -= 1;
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
    
    u64 elapsed_so_far = a->duration_us -
        a->remaining_duration_us;
    T1_log_assert(elapsed_so_far <=
        a->duration_us);
    
    f32 t_now = (f32)elapsed_so_far / (f32)a->duration_us;
    T1_log_assert(t_now <= 1.0f);
    T1_log_assert(t_now >= 0.0f);
    T1_log_assert(t_now >= a->already_applied_t);
    f32 t_applied = a->already_applied_t;
    
    T1_log_assert(a->already_applied_t <= t_now);
    a->already_applied_t = t_now;
    
    t_now = T1_easing_t_to_eased_t(
        t_now,
        a->easing_type);
    t_applied = T1_easing_t_to_eased_t(
        t_applied,
        a->easing_type);
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    T1_anim_sanity_check(a);
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    if (a->endpoints_not_deltas) {
        if (
            a->zs_gpu_f32s ||
            a->zs_gpu_u32s ||
            a->zs_cpu_f32s)
        {
            T1_zsprite_apply_endpoint_anim(
                /* u32 T1_id: */
                    a->target_T1_id,
                /* u32 touch_id: */
                    T1_TOUCH_ID_NONE,
                /* f32 t_applied: */
                    t_applied,
                /* f32 t_now: */
                    t_now,
                /* f32 * goal_gpu_vals_f32: */
                    (f32 *)a->zs_gpu_f32s,
                /* const u32 * goal_gpu_vals_u32: */
                    (u32 *)a->zs_gpu_u32s,
                /* const f32 * goal_cpu_vals: */
                    (f32 *)a->zs_cpu_f32s);
        }
        if (
            a->tq_gpu_f32s ||
            a->tq_gpu_u32s)
        {
            T1_texquad_apply_endpoint_anim(
                /* u32 T1_id: */
                    a->target_T1_id,
                /* u32 touch_id: */
                    T1_TOUCH_ID_NONE,
                /* f32 t_applied: */
                    t_applied,
                /* f32 t_now: */
                    t_now,
                /* const f32 * goal_gpu_vals_f32: */
                    (f32 *)a->tq_gpu_f32s,
                /* const u32 * goal_gpu_vals_u32: */
                    (u32 *)a->tq_gpu_u32s);
        }
        if (a->zl_gpu_f32s)
        {
            T1_zlight_apply_endpoint_anim(
                /* u32 T1_id: */
                    a->target_T1_id,
                /* f32 t_applied: */
                    t_applied,
                /* f32 t_now: */
                    t_now,
                /* const f32 * goal_gpu_vals_f32: */
                    (f32 *)a->zl_gpu_f32s);
        }
    } else {
        if (
            a->zs_gpu_f32s ||
            a->zs_gpu_u32s ||
            a->zs_cpu_f32s)
        {
            T1_zsprite_apply_anim_effects_to_id(
                /* u32 T1_id: */
                    a->target_T1_id,
                /* u32 touch_id: */
                    T1_TOUCH_ID_NONE,
                /* f32 t_applied: */
                    t_applied,
                /* f32 t_now: */
                    t_now,
                /* const f32 * anim_gpu_f32s: */
                    (f32 *)a->zs_gpu_f32s,
                /* const u32 * anim_gpu_u32s: */
                    (u32 *)a->zs_gpu_u32s,
                /* const f32 * anim_cpu_f32s: */
                    (f32 *)a->zs_cpu_f32s);
        }
        if (
            a->tq_gpu_f32s ||
            a->tq_gpu_u32s)
        {
            T1_texquad_apply_anim_effects_to_id(
                /* u32 T1_id: */
                    a->target_T1_id,
                /* u32 touch_id: */
                    T1_TOUCH_ID_NONE,
                /* f32 t_applied: */
                    t_applied,
                /* f32 t_now: */
                    t_now,
                /* const f32 * anim_gpu_vals_f32: */
                    (f32 *)a->tq_gpu_f32s,
                /* const u32 * anim_gpu_vals_u32: */
                    (u32 *)a->tq_gpu_u32s);
        }
        if (a->zl_gpu_f32s)
        {
            // TODO: apply anim (non-endpoint) for zLight
            T1_log_assert(0);
        }
    }
}

static void T1_anim_assert_anim_valid_before_commit(T1Anim * a)
{
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    T1_log_assert(!a->deleted);
    T1_log_assert(!a->committed);
    T1_log_assert(a->already_applied_t == 0.0f);
    T1_log_assert(a->duration_us > 0);
    T1_log_assert(a->target_T1_id != T1_ID_NONE);
    
    if (!a->endpoints_not_deltas) {
        // u32s jump to their endpoint, they can't be deltas
        T1_log_assert(!a->zs_gpu_u32s);
        T1_log_assert(!a->tq_gpu_u32s);
        
        if (a->zs_gpu_f32s) {
            // don't reduce alpha by 1 to fade out, use
            // an endpoint anim
            T1_log_assert(a->zs_gpu_f32s->alpha > -0.9f);
        }
    }
    
    T1_anim_sanity_check(a);
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    (void)a;
    #else
    #error
    #endif
}

// applies the whole anim right now, a has to have a 1us duration
static void T1_anim_commit_and_instarun(T1Anim * a)
{
    as->mutex_lock(as->mutex_id);
    
    T1_anim_assert_anim_valid_before_commit(a);
    T1_log_assert(a->duration_us == 1);
    
    a->remaining_pause_us = a->pause_us;
    a->remaining_duration_us = a->duration_us;
    a->committed = 1;
    
    T1_anim_resolve_single(a);
    T1_anim_delete(a);
    a->committed = 0;
    T1_log_assert(a->remaining_duration_us == 0);
    
    as->mutex_unlock(as->mutex_id);
}

static void T1_anim_commit_and_schedule(T1Anim * a)
{
    as->mutex_lock(as->mutex_id);
    
    T1_anim_assert_anim_valid_before_commit(a);
    
    if (a->del_conflict_anims)
    {
        for (
            s32 anim_i = (s32)T1_anims_size - 1;
            anim_i >= 0;
            anim_i--)
        {
            T1Anim * other = &T1_anims[anim_i];
            
            if (
                other != a &&
                (other->target_T1_id == a->target_T1_id ||
                other->target_T1_id == T1_ANIM_HIT_EVERYTHING) &&
                other->committed &&
                other->endpoints_not_deltas &&
                !other->del_obj_on_finish &&
                !other->run_func_on_finish)
            {
                T1_anim_delete(other);
            }
        }
    }
    
    a->remaining_pause_us = a->pause_us;
    a->remaining_duration_us = a->duration_us;
    a->committed = 1;
    
    as->mutex_unlock(as->mutex_id);
}

// shatter and evaporate are plain fades for now
static void T1_anim_alpha_blend_fade_and_destroy(
    u32 T1_id,
    u64 duration_us)
{
    T1_log_assert(duration_us > 0);
    T1_log_assert(T1_id != T1_ID_NONE);
    
    u32 set_alpha_on = T1_anim_new(T1_id, T1_ANIM_MODE_TO);
    T1_anim_set_f32(set_alpha_on, "alpha_on", 1.0f);
    T1_anim_set_duration_us(set_alpha_on, 1);
    T1_anim_commit(set_alpha_on);
    
    u32 fade = T1_anim_new(T1_id, T1_ANIM_MODE_TO);
    T1_anim_set_f32(fade, "alpha", 0.0f);
    T1_anim_set_duration_us(fade, duration_us);
    T1_anim_set_delete_target_on_finish(fade, 1);
    T1_anim_commit(fade);
}

void T1_anim_shatter_and_destroy(
    u32 T1_id,
    u64 duration_us)
{
    T1_anim_alpha_blend_fade_and_destroy(T1_id, duration_us);
}

void T1_anim_evaporate_and_destroy(
    u32 T1_id,
    u64 duration_us)
{
    T1_anim_alpha_blend_fade_and_destroy(T1_id, duration_us);
}

void T1_anim_fade_and_destroy(
    u32 T1_id,
    u64 pause_first,
    u64 duration_us)
{
    T1_log_assert(duration_us > 0);
    
    u32 fade_destroy = T1_anim_new(T1_id, T1_ANIM_MODE_TO);
    T1_anim_set_duration_us(fade_destroy, duration_us);
    T1_anim_set_pause_us(fade_destroy, pause_first);
    // zsprites
    T1_anim_set_f32(fade_destroy, "alpha", 0.0f);
    T1_anim_set_f32(fade_destroy, "bonus_rgb[0]", 0.0f);
    T1_anim_set_f32(fade_destroy, "bonus_rgb[1]", 0.0f);
    T1_anim_set_f32(fade_destroy, "bonus_rgb[2]", 0.0f);
    T1_anim_set_f32(fade_destroy, "shadow_strength", 0.0f);
    T1_anim_set_f32(fade_destroy, "bloom_on", 0.0f);
    // texquads and zlights
    T1_anim_set_f32(fade_destroy, "rgba[0]", 0.0f);
    T1_anim_set_f32(fade_destroy, "rgba[1]", 0.0f);
    T1_anim_set_f32(fade_destroy, "rgba[2]", 0.0f);
    T1_anim_set_f32(fade_destroy, "rgba[3]", 0.0f);
    // zlights
    T1_anim_set_f32(fade_destroy, "diffuse", 0.0f);
    T1_anim_set_f32(fade_destroy, "specular", 0.0f);
    T1_anim_set_delete_target_on_finish(fade_destroy, 1);
    T1_anim_commit(fade_destroy);
}

void T1_anim_fade_destroy_all(
    u64 pause_first,
    u64 duration_us)
{
    T1_anim_fade_and_destroy(
        /* u32 T1_id: */
            T1_ANIM_HIT_EVERYTHING,
        /* u64 pause_first_us: */
            pause_first,
        /* u64 duration_us: */
            duration_us);
}

void T1_anim_fade_to(
    u32 T1_id,
    u64 duration_us,
    f32 target_alpha)
{
    T1_log_assert(T1_id != T1_ID_NONE);
    
    u32 modify_alpha = T1_anim_new(T1_id, T1_ANIM_MODE_TO);
    // very short fades are applied right away
    T1_anim_set_duration_us(modify_alpha, duration_us < 50 ? 1 : duration_us);
    T1_anim_set_f32(modify_alpha, "alpha", target_alpha);
    T1_anim_set_delete_target_on_finish(modify_alpha, 1);
    T1_anim_commit(modify_alpha);
}

void T1_anim_resolve(void) {
    as->mutex_lock(as->mutex_id);
    
    for (
        s32 anim_i = (s32)T1_anims_size - 1;
        anim_i >= 0;
        anim_i--)
    {
        T1Anim * anim = &T1_anims[anim_i];
        
        T1_anim_resolve_single(anim);
    }
    
    as->mutex_unlock(as->mutex_id);
}

void T1_anim_dud_dance(
    u32 T1_id,
    f32 magnitude)
{
    u32 move_request = T1_anim_new(T1_id, T1_ANIM_MODE_BY);
    T1_anim_set_easing(
        move_request,
        T1_EASINGTYPE_QUADRUPLE_BOUNCE_ZERO_TO_ZERO);
    T1_anim_set_f32(move_request, "xyz[0]", magnitude * 0.05f);
    T1_anim_set_f32(move_request, "xyz[1]", magnitude * 0.035f);
    T1_anim_set_f32(move_request, "xyz[2]", magnitude * 0.005f);
    T1_anim_set_duration_us(move_request, 300000);
    T1_anim_commit(move_request);
}

void T1_anim_delete_all(void) {
    as->mutex_lock(as->mutex_id);
    for (u32 i = 0; i < T1_anims_size; i++) {
        T1_anim_delete(T1_anims + i);
    }
    as->mutex_unlock(as->mutex_id);
}

static T1Anim * T1_anim_from_handle(u32 anim)
{
    T1_log_assert(anim < T1_anims_size);
    T1Anim * a = &T1_anims[anim];
    // handles are only valid until they're committed
    T1_log_assert(!a->deleted);
    T1_log_assert(!a->committed);
    return a;
}

u32 T1_anim_new(u32 T1_id, u8 mode)
{
    T1_log_assert(T1_id != T1_ID_NONE);
    T1_log_assert(mode == T1_ANIM_MODE_TO || mode == T1_ANIM_MODE_BY);
    
    T1Anim * a = T1_anim_alloc(
        /* b8 endpoints_not_deltas: */ mode == T1_ANIM_MODE_TO);
    a->target_T1_id = T1_id;
    
    return (u32)(a - T1_anims);
}

// fetches the anim's store for this kind of property on first use
static void * T1_anim_get_store(
    T1Anim * a,
    const u8 store)
{
    void * out = NULL;
    u32 size = 0;
    
    switch (store) {
        case T1_PROP_STORE_ZS_CPU_F32:
            if (a->zs_cpu_f32s) { return a->zs_cpu_f32s; }
            a->zs_cpu_f32s = T1_anim_fetch_next_store_zs_cpu_f32s(
                &a->zs_cpu_f32s_store_i);
            out = a->zs_cpu_f32s;
            size = sizeof(T1CPUzSpritef32);
            break;
        case T1_PROP_STORE_ZS_GPU_F32:
            if (a->zs_gpu_f32s) { return a->zs_gpu_f32s; }
            a->zs_gpu_f32s = T1_anim_fetch_next_store_zs_gpu_f32s(
                &a->zs_gpu_f32s_store_i);
            out = a->zs_gpu_f32s;
            size = sizeof(T1GPUzSpritef32);
            break;
        case T1_PROP_STORE_ZS_GPU_U32:
            if (a->zs_gpu_u32s) { return a->zs_gpu_u32s; }
            a->zs_gpu_u32s = T1_anim_fetch_next_store_zs_gpu_u32s(
                &a->zs_gpu_u32s_store_i);
            out = a->zs_gpu_u32s;
            size = sizeof(T1GPUzSpriteu32);
            break;
        case T1_PROP_STORE_TQ_GPU_F32:
            if (a->tq_gpu_f32s) { return a->tq_gpu_f32s; }
            a->tq_gpu_f32s = T1_anim_fetch_next_store_tq_gpu_f32s(
                &a->tq_gpu_f32s_store_i);
            out = a->tq_gpu_f32s;
            size = sizeof(T1GPUTexQuadf32);
            break;
        case T1_PROP_STORE_TQ_GPU_U32:
            if (a->tq_gpu_u32s) { return a->tq_gpu_u32s; }
            a->tq_gpu_u32s = T1_anim_fetch_next_store_tq_gpu_u32s(
                &a->tq_gpu_u32s_store_i);
            out = a->tq_gpu_u32s;
            size = sizeof(T1GPUTexQuadu32);
            break;
        case T1_PROP_STORE_ZL_F32:
            if (a->zl_gpu_f32s) { return a->zl_gpu_f32s; }
            a->zl_gpu_f32s = T1_anim_fetch_next_store_zl_gpu_f32s(
                &a->zl_f32s_store_i);
            out = a->zl_gpu_f32s;
            size = sizeof(T1zLightf32);
            break;
        default:
            T1_log_assert(0);
            return NULL;
    }
    
    // fetched stores are zeroed, which is "no effect" for deltas
    if (a->endpoints_not_deltas) {
        T1_std_memset_f32(out, T1_ANIM_NO_EFFECT, size);
    }
    
    return out;
}

static void T1_anim_set_bits(
    const u32 anim,
    const c8 * name,
    const u8 is_u32,
    const u32 bits)
{
    T1Anim * a = T1_anim_from_handle(anim);
    
    const T1Property * matches[T1_PROPERTY_MATCHES_CAP];
    u32 matches_size = T1_property_find(
        name, is_u32, 0, matches, T1_PROPERTY_MATCHES_CAP);
    
    as->mutex_lock(as->mutex_id);
    
    u32 stores_set = 0;
    for (u32 i = 0; i < matches_size; i++) {
        const T1Property * p = matches[i];
        
        // e.g. texquad offsets and materials have no anim store
        if (!T1_property_is_animatable(p)) { continue; }
        // TODO: deltas aren't implemented for zlights
        if (
            !a->endpoints_not_deltas &&
            p->store == T1_PROP_STORE_ZL_F32)
        {
            continue;
        }
        // TODO: animate values that share a u32
        T1_log_assert(!T1_property_is_packed(p));
        // u32s jump to their endpoint, they can't be deltas
        T1_log_assert(a->endpoints_not_deltas || !p->is_u32);
        
        void * store_struct = T1_anim_get_store(a, p->store);
        T1_property_write(store_struct, p, bits);
        stores_set += 1;
    }
    
    as->mutex_unlock(as->mutex_id);
    
    // this property can't be animated (in this mode)
    T1_log_assert(stores_set > 0);
}

void T1_anim_set_f32(u32 anim, const c8 * property, f32 value)
{
    u32 bits;
    T1_std_memcpy(&bits, &value, 4);
    T1_anim_set_bits(anim, property, 0, bits);
}

void T1_anim_set_u32(u32 anim, const c8 * property, u32 value)
{
    T1_anim_set_bits(anim, property, 1, value);
}

void T1_anim_set_duration_us(u32 anim, u64 duration_us) {
    T1_anim_from_handle(anim)->duration_us = duration_us;
}

void T1_anim_set_pause_us(u32 anim, u64 pause_us) {
    T1_anim_from_handle(anim)->pause_us = pause_us;
}

void T1_anim_set_easing(u32 anim, T1EasingType easing_type) {
    T1_anim_from_handle(anim)->easing_type = easing_type;
}

void T1_anim_set_runs(u32 anim, u32 runs) {
    T1_anim_from_handle(anim)->runs = runs;
}

void T1_anim_set_delete_target_on_finish(u32 anim, u8 delete_target) {
    T1_anim_from_handle(anim)->del_obj_on_finish = delete_target;
}

void T1_anim_set_cancel_conflicting(u32 anim, u8 cancel_conflicting) {
    T1_anim_from_handle(anim)->del_conflict_anims = cancel_conflicting;
}

void T1_anim_set_run_func_on_finish(u32 anim, void (* func)(void)) {
    T1_anim_from_handle(anim)->run_func_on_finish = func;
}

void T1_anim_commit(u32 anim) {
    T1Anim * a = T1_anim_from_handle(anim);
    if (a->duration_us == 1) {
        T1_anim_commit_and_instarun(a);
    } else {
        T1_anim_commit_and_schedule(a);
    }
}

void T1_anim_rename_target_T1_id(u32 from_T1_id, u32 to_T1_id) {
    as->mutex_lock(as->mutex_id);
    for (u32 i = 0; i < T1_anims_size; i++) {
        if (
            !T1_anims[i].deleted &&
            T1_anims[i].target_T1_id == from_T1_id)
        {
            T1_anims[i].target_T1_id = to_T1_id;
        }
    }
    as->mutex_unlock(as->mutex_id);
}

void T1_anim_delete_all_anims_targeting(u32 T1_id) {
    as->mutex_lock(as->mutex_id);
    for (u32 i = 0; i < T1_anims_size; i++) {
        T1Anim * a = &T1_anims[i];
        
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

