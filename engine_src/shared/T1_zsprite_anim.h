#ifndef T1_ZSPRITE_ANIM_H
#define T1_ZSPRITE_ANIM_H

#if T1_ZSPRITE_ANIM_ACTIVE == T1_ACTIVE

#include "T1_std.h"
#include "T1_simd.h"

#include "T1_easing.h"
#include "T1_zsprite.h"
#include "T1_particle.h"
// #include "clientlogic.h"


void T1_zsprite_anim_init(void);

void T1_zsprite_anim_resolve(void);

typedef struct {
    T1GPUzSprite gpu_vals;
    T1CPUzSpriteSimdStats cpu_vals;
    
    uint64_t duration_us;
    uint64_t pause_us;
    
    uint32_t runs;
    int32_t affected_zsprite_id;
    int32_t affected_touchable_id;
    
    T1EasingType easing_type; // u8
    bool8_t delete_object_when_finished;
    bool8_t delete_other_anims_targeting_zsprite;
} T1zSpriteAnim;

T1zSpriteAnim * T1_zsprite_anim_request_next(
    bool8_t endpoints_not_deltas);

void T1_zsprite_anim_commit(
    T1zSpriteAnim * to_commit);

void T1_zsprite_anim_evaporate_and_destroy(
    const int32_t zsprite_id,
    const uint64_t duration_us);

void T1_zsprite_anim_shatter_and_destroy(
    const int32_t zsprite_id,
    const uint64_t duration_us);

void T1_zsprite_anim_fade_and_destroy(
    const int32_t zsprite_id,
    const uint64_t duration_us);

void T1_zsprite_anim_fade_to(
    const int32_t zsprite_id,
    const uint64_t duration_us,
    const float target_alpha);

void T1_zsprite_anim_dud_dance(
    const int32_t zsprite_id,
    const float magnitude);

void T1_zsprite_anim_bump(
    const int32_t zsprite_id,
    const uint32_t wait);

void T1_zsprite_anim_delete_all(void);
void T1_zsprite_anim_delete_endpoint_anims_targeting(
    const int32_t zsprite_id);
void T1_zsprite_anim_delete_all_anims_targeting(const int32_t zsprite_id);

void T1_zsprite_anim_set_ignore_camera_but_retain_screenspace_pos(
    const int32_t zsprite_id,
    const float new_ignore_camera);

#elif T1_ZSPRITE_ANIM_ACTIVE == T1_INACTIVE
#else
#error
#endif

#endif // T1_ZSPRITE_ANIM_H
