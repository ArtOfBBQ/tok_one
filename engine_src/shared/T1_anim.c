#include "T1_anim.h"
#include "T1_pipeline.h"
#include "T1_pass.h"

#include <stddef.h>

#include "T1_mem.h"
#include "T1_log.h"
#include "T1_tex.h"
#include "T1_global.h"
#include "T1_types_cpu_to_gpu.h"
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
    // indexed by T1_PROP_STORE_*, NULL for the stores this anim doesn't use
    void * stores[T1_PROP_STORE_OUTOFBOUNDS];
    
    u64 duration_us;
    u64 pause_us;
    u64 remaining_duration_us;
    u64 remaining_pause_us;
    f32 already_applied_t;
    u16 stores_i[T1_PROP_STORE_OUTOFBOUNDS]; // their slots in the pools
    u32 runs;
    u32 target_T1_id;
    f32 (* easing)(f32 t); // NULL is linear
    b8 del_obj_on_finish;
    b8 del_conflict_anims;
    b8 endpoints_not_deltas;
    b8 deleted;
    b8 committed;
} T1Anim;

T1Anim * T1_anims = NULL;
u32 T1_anims_size = 0;

// the struct each animatable store animates, 0 for stores anims can't touch
static const u32 T1_anim_store_sizes[T1_PROP_STORE_OUTOFBOUNDS] = {
    [T1_PROP_STORE_ZS_CPU_F32] = sizeof(T1CPUzSpritef32),
    [T1_PROP_STORE_ZS_GPU_F32] = sizeof(T1GPUzSpritef32),
    [T1_PROP_STORE_ZS_GPU_U32] = sizeof(T1GPUzSpriteu32),
    [T1_PROP_STORE_TQ_GPU_F32] = sizeof(T1GPUTexQuadf32),
    [T1_PROP_STORE_TQ_GPU_U32] = sizeof(T1GPUTexQuadu32),
    [T1_PROP_STORE_ZL_F32]     = sizeof(T1zLightf32),
    [T1_PROP_STORE_ZL_U32]     = sizeof(T1zLightu32),
    [T1_PROP_STORE_CAM_F32]    = sizeof(T1CamF32s),
};

static b8 T1_anim_store_is_u32(const u8 store) {
    return
        store == T1_PROP_STORE_ZS_GPU_U32 ||
        store == T1_PROP_STORE_TQ_GPU_U32 ||
        store == T1_PROP_STORE_ZL_U32;
}

#define T1_ANIM_STORE(a, store, type) ((type *)(a)->stores[store])


#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
static void T1_anim_sanity_check(T1Anim * a) {
    T1_log_assert(a->remaining_duration_us < 195000000);
    
    T1CPUzSpritef32 * zs_cpu =
        T1_ANIM_STORE(a, T1_PROP_STORE_ZS_CPU_F32, T1CPUzSpritef32);
    T1GPUzSpriteu32 * zs_u32 =
        T1_ANIM_STORE(a, T1_PROP_STORE_ZS_GPU_U32, T1GPUzSpriteu32);
    
    if (!a->endpoints_not_deltas) {
        if (zs_cpu) {
            T1_log_assert(zs_cpu->bloom_on != T1_ANIM_NO_EFFECT);
        }
        T1_log_assert(zs_u32 == NULL);
    } else if (zs_u32) {
        f32 mix_rv_and_mix_tex_f32;
        T1_std_memcpy(
            &mix_rv_and_mix_tex_f32,
            &zs_u32->mix_rv_and_mix_tex,
            4);
        if (mix_rv_and_mix_tex_f32 != T1_ANIM_NO_EFFECT) {
            // like the shader: rv is the low 16 bits, tex the high 16
            s16 mix_rv = (s16)(zs_u32->mix_rv_and_mix_tex & 0xFFFF);
            T1_log_assert(mix_rv < T1_RENDER_VIEW_CAP);
            T1_log_assert(mix_rv >= -1);
            
            T1Tex mix_tex = (T1Tex)(zs_u32->mix_rv_and_mix_tex >> 16);
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
    // per store: T1_ANIMS_CAP slots of T1_anim_store_sizes[store] bytes
    u8 * pools[T1_PROP_STORE_OUTOFBOUNDS];
    b8 * pools_taken[T1_PROP_STORE_OUTOFBOUNDS];
    u32 (* init_mutex_and_return_id)(void);
    void (* mutex_lock)(u32);
    void (* mutex_unlock)(u32);
    u32 mutex_id;
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
    
    for (u32 store = 0; store < T1_PROP_STORE_OUTOFBOUNDS; store++) {
        if (T1_anim_store_sizes[store] == 0) { continue; }
        
        as->pools[store] = T1_mem_malloc_unmanaged(
            T1_anim_store_sizes[store] * T1_ANIMS_CAP);
        as->pools_taken[store] = T1_mem_malloc_unmanaged(
            sizeof(b8) * T1_ANIMS_CAP);
        T1_std_memset(
            as->pools_taken[store],
            0,
            sizeof(b8) * T1_ANIMS_CAP);
    }
    
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

// a zeroed slot in the store's pool
static void * T1_anim_fetch_next_store(
    const u8 store,
    u16 * store_i)
{
    T1_log_assert(T1_anim_store_sizes[store] > 0);
    
    u16 i = 0;
    while (i < T1_ANIMS_CAP && as->pools_taken[store][i]) {
        i++;
    }
    T1_log_assert(i < T1_ANIMS_CAP);
    
    *store_i = i;
    as->pools_taken[store][i] = 1;
    void * out = as->pools[store] + (T1_anim_store_sizes[store] * i);
    T1_std_memset(out, 0, T1_anim_store_sizes[store]);
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
    for (u32 store = 0; store < T1_PROP_STORE_OUTOFBOUNDS; store++) {
        if (a->stores[store]) {
            as->pools_taken[store][a->stores_i[store]] = 0;
        }
    }
    
    a->deleted = 1;
    a->committed = 0;
    
    if (a == (T1_anims + (s32)T1_anims_size-1)) { T1_anims_size -= 1; }
}

// moves a forward by elapsed microseconds
// applies one of a's stores (if a has it) to every object a targets
static void T1_anim_apply_store(
    const T1Anim * a,
    T1AnimStep * step,
    const u8 store)
{
    if (a->stores[store] == NULL) { return; }
    
    step->vals = a->stores[store];
    step->size_bytes = T1_anim_store_sizes[store];
    step->is_u32 = T1_anim_store_is_u32(store);
    
    switch (store) {
        case T1_PROP_STORE_ZS_CPU_F32:
        case T1_PROP_STORE_ZS_GPU_F32:
        case T1_PROP_STORE_ZS_GPU_U32:
            T1_zsprite_apply_anim_step(a->target_T1_id, store, step);
            break;
        case T1_PROP_STORE_TQ_GPU_F32:
        case T1_PROP_STORE_TQ_GPU_U32:
            T1_texquad_apply_anim_step(a->target_T1_id, store, step);
            break;
        case T1_PROP_STORE_ZL_F32:
        case T1_PROP_STORE_ZL_U32:
            T1_zlight_apply_anim_step(a->target_T1_id, store, step);
            break;
        case T1_PROP_STORE_CAM_F32:
            T1_render_view_apply_anim_step(a->target_T1_id, store, step);
            break;
        default:
            T1_log_assert(0);
    }
}

static void T1_anim_resolve_single(
    T1Anim * a,
    u64 elapsed)
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
    // decided before easing, an eased t can end anywhere
    b8 finished = t_now >= 1.0f;
    
    if (a->easing) {
        t_now = a->easing(t_now);
        t_applied = a->easing(t_applied);
    }
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    T1_anim_sanity_check(a);
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1AnimStep step;
    step.t_applied = t_applied;
    step.t_now = t_now;
    step.endpoints_not_deltas = a->endpoints_not_deltas;
    step.finished = finished;
    
    for (u8 store = 0; store < T1_PROP_STORE_OUTOFBOUNDS; store++) {
        T1_anim_apply_store(a, &step, store);
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
        for (u8 store = 0; store < T1_PROP_STORE_OUTOFBOUNDS; store++) {
            T1_log_assert(!a->stores[store] || !T1_anim_store_is_u32(store));
        }
        
        T1GPUzSpritef32 * zs_f32 =
            T1_ANIM_STORE(a, T1_PROP_STORE_ZS_GPU_F32, T1GPUzSpritef32);
        if (zs_f32) {
            // don't reduce alpha by 1 to fade out, use
            // an endpoint anim
            T1_log_assert(zs_f32->alpha > -0.9f);
        }
    }
    
    T1_anim_sanity_check(a);
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    (void)a;
    #else
    #error
    #endif
}

// unused values are T1_ANIM_NO_EFFECT in an endpoint store, 0 in a delta store
static f32 T1_anim_unused_val(const T1Anim * a) {
    return a->endpoints_not_deltas ? T1_ANIM_NO_EFFECT : 0.0f;
}

// whether both stores animate at least one of the same values
static b8 T1_anim_stores_overlap(
    const T1Anim * a,
    const void * a_store,
    const T1Anim * b,
    const void * b_store,
    const u32 size)
{
    if (a_store == NULL || b_store == NULL) { return 0; }
    
    const f32 * a_vals = (const f32 *)a_store;
    const f32 * b_vals = (const f32 *)b_store;
    f32 a_unused = T1_anim_unused_val(a);
    f32 b_unused = T1_anim_unused_val(b);
    for (u32 i = 0; i < size / sizeof(f32); i++) {
        if (a_vals[i] != a_unused && b_vals[i] != b_unused) {
            return 1;
        }
    }
    
    return 0;
}

static b8 T1_anim_shares_a_property(const T1Anim * a, const T1Anim * b)
{
    for (u8 store = 0; store < T1_PROP_STORE_OUTOFBOUNDS; store++) {
        if (
            T1_anim_stores_overlap(
                a, a->stores[store],
                b, b->stores[store],
                T1_anim_store_sizes[store]))
        {
            return 1;
        }
    }
    
    return 0;
}

// deletes the other committed endpoint anims that animate a's properties
// on a's T1_id
static void T1_anim_delete_conflicting(const T1Anim * a)
{
    if (!a->del_conflict_anims) { return; }
    
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
            T1_anim_shares_a_property(a, other))
        {
            T1_anim_delete(other);
        }
    }
}

// applies the whole anim right now, a has to have a 1us duration and no pause
static void T1_anim_commit_and_instarun(T1Anim * a)
{
    as->mutex_lock(as->mutex_id);
    
    T1_anim_assert_anim_valid_before_commit(a);
    T1_log_assert(a->duration_us == 1);
    T1_log_assert(a->pause_us == 0);
    
    T1_anim_delete_conflicting(a);
    
    a->remaining_pause_us = a->pause_us;
    a->remaining_duration_us = a->duration_us;
    a->committed = 1;
    
    // all of it, whether or not a frame has passed
    T1_anim_resolve_single(a, a->pause_us + a->duration_us);
    T1_anim_delete(a);
    a->committed = 0;
    T1_log_assert(a->remaining_duration_us == 0);
    
    as->mutex_unlock(as->mutex_id);
}

static void T1_anim_commit_and_schedule(T1Anim * a)
{
    as->mutex_lock(as->mutex_id);
    
    T1_anim_assert_anim_valid_before_commit(a);
    
    T1_anim_delete_conflicting(a);
    
    a->remaining_pause_us = a->pause_us;
    a->remaining_duration_us = a->duration_us;
    a->committed = 1;
    
    as->mutex_unlock(as->mutex_id);
}

// shatter and evaporate are plain fades for now
void T1_anim_resolve(void) {
    as->mutex_lock(as->mutex_id);
    
    for (
        s32 anim_i = (s32)T1_anims_size - 1;
        anim_i >= 0;
        anim_i--)
    {
        T1Anim * anim = &T1_anims[anim_i];
        
        T1_anim_resolve_single(anim, T1_global->elapsed);
    }
    
    as->mutex_unlock(as->mutex_id);
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
    // pipelines and passes have nothing to animate
    T1_log_assert(T1_pipeline_T1_id_to_i(T1_id) < 0);
    T1_log_assert(T1_pass_T1_id_to_i(T1_id) < 0);
    
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
    if (a->stores[store]) { return a->stores[store]; }
    
    void * out = T1_anim_fetch_next_store(store, &a->stores_i[store]);
    a->stores[store] = out;
    
    // fetched stores are zeroed, which is "no effect" for deltas
    if (a->endpoints_not_deltas) {
        T1_std_memset_f32(out, T1_ANIM_NO_EFFECT, T1_anim_store_sizes[store]);
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
        name, is_u32, T1_PROP_STORE_OUTOFBOUNDS,
        matches, T1_PROPERTY_MATCHES_CAP);
    
    as->mutex_lock(as->mutex_id);
    
    u32 stores_set = 0;
    for (u32 i = 0; i < matches_size; i++) {
        const T1Property * p = matches[i];
        
        // e.g. texquad offsets and materials have no anim store
        if (!T1_property_is_animatable(p)) { continue; }
        // TODO: animate values that share a u32
        T1_log_assert(!T1_property_is_packed(p));
        // u32s jump to their endpoint, they can't be deltas
        T1_log_assert(a->endpoints_not_deltas || !p->is_u32);
        
        void * store_struct = T1_anim_get_store(a, p->store);
        T1_property_write(
            store_struct,
            p,
            T1_property_convert_bits(p, bits));
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

void T1_anim_set_easing(u32 anim, f32 (* easing)(f32 t)) {
    T1_anim_from_handle(anim)->easing = easing;
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

void T1_anim_commit(u32 anim) {
    T1Anim * a = T1_anim_from_handle(anim);
    if (a->duration_us == 1 && a->pause_us == 0) {
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

