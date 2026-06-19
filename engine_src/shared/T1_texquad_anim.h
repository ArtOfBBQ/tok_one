#ifndef T1_TEXQUAD_ANIM_H
#define T1_TEXQUAD_ANIM_H

#if T1_TEXQUAD_ANIM_ACTIVE == T1_ACTIVE

#include "T1_std.h"
#include "T1_cpu_to_gpu.h"
#include "T1_easing.h"

void T1_texquad_anim_init(
    u32 (* funcptr_init_mutex_and_return_id)(void),
    void (* funcptr_mutex_lock)(const u32),
    void (* funcptr_mutex_unlock)(const u32));

void T1_texquad_anim_resolve(void);

typedef struct {
    T1GPUTexQuad gpu_vals;
    
    u64 duration_us;
    u64 pause_us;
    
    u32 runs;
    s32 affect_T1_id;
    s32 affect_touch_id;
    
    T1EasingType easing_type; // u8
    b8 gpu_f32_active;
    b8 gpu_s32_active;
    b8 del_obj_on_finish;
    b8 del_conflict_anims;
} T1TexQuadAnim;

T1TexQuadAnim * T1_texquad_anim_request_next(
    b8 endpoints_not_deltas);

void T1_texquad_anim_commit(
    T1TexQuadAnim * to_commit);

void T1_texquad_anim_commit_and_instarun(
    T1TexQuadAnim * to_commit);

void
T1_texquad_anim_fade_to(
    const s32 T1_id,
    const u64 duration_us,
    const f32 target_alpha);

void T1_texquad_anim_fade_and_destroy(
    const s32 T1_id,
    const u64 duration_us);

void T1_texquad_anim_fade_destroy_all(
    const u64 duration_us);

void T1_texquad_anim_delete_all(void);

#elif T1_TEXQUAD_ANIM_ACTIVE == T1_INACTIVE
#else
#error
#endif

#endif // T1_TEXQUAD_ANIM_H
