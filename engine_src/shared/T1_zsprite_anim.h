#ifndef T1_ZSPRITE_ANIM_H
#define T1_ZSPRITE_ANIM_H

#if T1_ZSPRITE_ANIM_ACTIVE == T1_ACTIVE

#include "T1_std.h"
#include "T1_simd.h"
#include "T1_zsprite.h"
#include "T1_easing.h"


void T1_zsprite_anim_init(
    u32 (* fptr_init_mutex_and_return_id)(void),
    void (* fptr_mutex_lock)(const u32),
    void (* fptr_mutex_unlock)(const u32));

void T1_zsprite_anim_resolve(void);

typedef struct {
    T1GPUzSprite gpu_vals;
    T1CPUzSpriteSimdStats cpu_vals;
    
    u64 duration_us;
    u64 pause_us;
    
    u32 runs;
    s32 affected_T1_id;
    s32 affected_touch_id;
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    char original_func_name[128];
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1EasingType easing_type; // u8
    b8 gpu_vals_f32_active;
    b8 gpu_vals_s32_active;
    b8 cpu_vals_active;
    b8 del_obj_on_finish;
    b8 del_conflict_anims;
} T1zSpriteAnim;

T1zSpriteAnim * T1_zsprite_anim_request_next(
    b8 endpoints_not_deltas);

void T1_zsprite_anim_commit_and_instarun(
    T1zSpriteAnim * to_commit
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    ,const char * original_func_name
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    );

void
T1_zsprite_anim_assert_anim_valid_before_commit(
    T1zSpriteAnim * to_check);

void
T1_zsprite_anim_commit(
    T1zSpriteAnim * to_commit
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    ,const char * original_func_name
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    );

void
T1_zsprite_anim_shatter_and_destroy(
    const s32 T1_id,
    const u64 duration_us);

void
T1_zsprite_anim_evaporate_and_destroy(
    const s32 T1_id,
    const u64 duration_us);

void
T1_zsprite_anim_fade_and_destroy(
    const s32 T1_id,
    const u64 duration_us);

void
T1_zsprite_anim_fade_destroy_all(
    const u64 duration_us);

void
T1_zsprite_anim_fade_to(
    const s32 T1_id,
    const u64 duration_us,
    const f32 target_alpha);

void
T1_zsprite_anim_dud_dance(
    const s32 T1_id,
    const f32 magnitude);

void
T1_zsprite_anim_bump(
    const s32 T1_id,
    const u32 wait);

void
T1_zsprite_anim_delete_all(void);

void
T1_zsprite_anim_delete_endpoint_anims_targeting(
    const s32 T1_id);

void
T1_zsprite_anim_delete_all_anims_targeting(const
    s32 T1_id);

void
T1_zsprite_anim_set_ignore_camera_but_retain_screenspace_pos(
    const s32 T1_id,
    const f32 new_ignore_camera);

#elif T1_ZSPRITE_ANIM_ACTIVE == T1_INACTIVE
#else
#error
#endif

#endif // T1_ZSPRITE_ANIM_H
