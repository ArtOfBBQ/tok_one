#ifndef T1_ANIM_H
#define T1_ANIM_H

#if T1_ANIM_ACTIVE == T1_ACTIVE

#include "T1_std.h"
#include "T1_simd.h"
#include "T1_types_public.h"
#include "T1_easing.h"


void T1_anim_init(
    u32 (* fptr_init_mutex_and_return_id)(void),
    void (* fptr_mutex_lock)(u32),
    void (* fptr_mutex_unlock)(u32));

void T1_anim_resolve(void);

typedef struct {
    T1GPUzSpritef32 * zs_gpu_f32s;
    T1CPUzSpritef32 * zs_cpu_f32s;
    T1GPUzSprites32 * zs_gpu_s32s;
    T1GPUTexQuadf32 * tq_gpu_f32s;
    T1GPUTexQuads32 * tq_gpu_s32s;
    
    u64 duration_us;
    u64 pause_us;
    
    u32 runs;
    s32 target_T1_id;
    s32 target_touch_id;
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    char original_func_name[128];
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1EasingType easing_type; // u8
    b8 del_obj_on_finish;
    b8 del_conflict_anims;
} T1Anim;

T1Anim * T1_anim_request_next(
    b8 endpoints_not_deltas,
    b8 zs_gpu_f32s,
    b8 zs_cpu_f32s,
    b8 zs_gpu_s32s,
    b8 tq_gpu_f32s,
    b8 tq_gpu_s32s);

void T1_anim_commit_and_instarun(
    T1Anim * to_commit
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    ,const char * original_func_name
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    );

void T1_anim_assert_anim_valid_before_commit(T1Anim * to_check);

void T1_anim_commit(
    T1Anim * to_commit
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    ,const char * original_func_name
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    );

void T1_anim_shatter_and_destroy(
    s32 T1_id,
    u64 duration_us);

void T1_anim_evaporate_and_destroy(
    s32 T1_id,
    u64 duration_us);

void T1_anim_fade_and_destroy(
    s32 T1_id,
    u64 duration_us);

void T1_anim_fade_destroy_all(u64 duration_us);

void T1_anim_fade_to(
    s32 T1_id,
    u64 duration_us,
    f32 target_alpha);

void T1_anim_dud_dance(
    s32 T1_id,
    f32 magnitude);

void T1_anim_bump(
    s32 T1_id,
    u32 wait);

void T1_anim_delete_all(void);

void T1_anim_delete_all_anims_targeting(const
    s32 T1_id);

void T1_anim_set_ignore_camera_but_retain_screenspace_pos(
    s32 T1_id,
    f32 new_ignore_camera);

#elif T1_ANIM_ACTIVE == T1_INACTIVE
#else
#error
#endif

#endif // T1_ANIM_H
