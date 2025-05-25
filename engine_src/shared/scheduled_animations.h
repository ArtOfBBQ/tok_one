#if SCHEDULED_ANIMS_ACTIVE

#ifndef SCHEDULED_ANIMATION_H
#define SCHEDULED_ANIMATION_H

#include "common.h"
#include "simd.h"

#include "zpolygon.h"
#include "particle.h"
// #include "clientlogic.h"

#ifdef __cplusplus
extern "C" {
#endif

void scheduled_animations_init(void);

float scheduled_animations_easing_bounce_zero_to_zero(
    const float t,
    const float bounces);
float scheduled_animations_easing_pulse_zero_to_zero(
    const float t,
    const float pulses);
float scheduled_animations_easing_out_elastic_zero_to_one(const float t);
float scheduled_animations_easing_out_quart(const float t);
float scheduled_animations_easing_lin_revert(const float t);

void scheduled_animations_resolve(void);

typedef enum EasingType {
    EASINGTYPE_NONE = 0,
    EASINGTYPE_EASEOUT_ELASTIC_ZERO_TO_ONE,
    EASINGTYPE_SINGLE_BOUNCE_ZERO_TO_ZERO,
    EASINGTYPE_DOUBLE_BOUNCE_ZERO_TO_ZERO,
    EASINGTYPE_QUADRUPLE_BOUNCE_ZERO_TO_ZERO,
    EASINGTYPE_OCTUPLE_BOUNCE_ZERO_TO_ZERO,
    EASINGTYPE_SINGLE_PULSE_ZERO_TO_ZERO,
    EASINGTYPE_OCTUPLE_PULSE_ZERO_TO_ZERO,
} EasingType;

typedef struct ScheduledAnimation {
    // Public:
    GPUzSprite         gpu_polygon_vals;
    // zPolygonCPU        zpolygon_cpu_vals;
    GPUSpriteMaterial gpu_polygon_material_vals;
    zLightSource       lightsource_vals;
    
    GPUSpriteMaterial onfinish_gpu_polygon_material_muls;
    GPUSpriteMaterial onfinish_gpu_polygon_material_adds;
    
    EasingType easing_type;
    
    uint64_t duration_microseconds;
    
    uint32_t runs;
    int32_t affected_sprite_id;
    int32_t affected_touchable_id;
    bool32_t delete_object_when_finished;
    
    /*
    PRIVATE:
    */
    
    /*
    t is a value from 0.0f to 1.0f.
    if already_applied_t is 0.2f, that means we already applied the
    effects from the animation's run from t=0.0f to t=0.2f, and we are
    waiting to apply the remaining 80% of the effects. This is meant
    to be unrelated from frame speed - if we had a very slow frame we
    may have to suddenly apply 90% of an animation that was supposed to run
    over a whole second.
    */
    float already_applied_t;
    
    /*
    If this flag is set, the target values represent endpoints, not deltas,
    until the animation is committed. During scheduled_animation_commit, the
    values will be converted to normal delta values, and the animation will
    be treated just like any other animation
    */
    uint64_t start_timestamp;
    uint64_t end_timestamp;
    
    bool32_t endpoints_not_deltas;
    
    bool32_t delete_other_anims_targeting_same_object_id_on_commit;
    bool32_t deleted;
    bool32_t committed;
} ScheduledAnimation;

ScheduledAnimation * scheduled_animations_request_next(
    bool32_t endpoints_not_deltas);

void scheduled_animations_commit(ScheduledAnimation * to_commit);

void scheduled_animations_request_evaporate_and_destroy(
    const int32_t object_id,
    const uint64_t duration_microseconds);

void scheduled_animations_request_shatter_and_destroy(
    const int32_t object_id,
    const uint64_t duration_microseconds);

void scheduled_animations_request_fade_and_destroy(
    const int32_t object_id,
    const uint64_t duration_microseconds);

void scheduled_animations_request_fade_to(
    const int32_t object_id,
    const uint64_t duration_microseconds,
    const float target_alpha);

void scheduled_animations_request_dud_dance(
    const int32_t object_id,
    const float magnitude);

void scheduled_animations_request_bump(
    const int32_t object_id,
    const uint32_t wait);

void scheduled_animations_delete_all(void);
void scheduled_animations_delete_endpoint_anims_targeting(
    const int32_t object_id);
void scheduled_animations_delete_all_anims_targeting(const int32_t object_id);

#ifdef __cplusplus
}
#endif

#endif // SCHEDULED_ANIMATION_H

#endif // SCHEDULED_ANIMS_ACTIVE
