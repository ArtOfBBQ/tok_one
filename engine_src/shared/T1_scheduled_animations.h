#if SCHEDULED_ANIMS_ACTIVE

#ifndef SCHEDULED_ANIMATION_H
#define SCHEDULED_ANIMATION_H

#include "T1_std.h"
#include "T1_simd.h"

#include "T1_zsprite.h"
#include "T1_particle.h"
// #include "clientlogic.h"

#ifdef __cplusplus
extern "C" {
#endif

void T1_scheduled_animations_init(void);

void T1_scheduled_animations_resolve(void);

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

/*
if t.now is 0.9f and t.applied is 0.25f, that means we're currently
at the 90% point of the animation and we already previously applied the
effects of the animation up until the 25% point
*/
typedef struct T1TPair {
    float now;
    float applied;
} T1TPair;

typedef struct T1ScheduledAnimation {
    // Public:
    GPUzSprite gpu_polygon_vals;
    // zPolygonCPU zpolygon_cpu_vals;
    zLightSource lightsource_vals;
    
    EasingType easing_type;
    
    uint64_t duration_us;
    uint64_t pause_us;
    
    uint32_t runs;
    int32_t affected_zsprite_id;
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
    uint64_t remaining_duration_us;
    uint64_t remaining_pause_us;
    float already_applied_t;
    
    /*
    If this flag is set, the target values represent endpoints, not deltas,
    until the animation is committed. During scheduled_animation_commit, the
    values will be converted to normal delta values, and the animation will
    be treated just like any other animation
    */
    
    bool32_t endpoints_not_deltas;
    
    bool32_t delete_other_anims_targeting_same_object_id_on_commit;
    bool32_t deleted;
    bool32_t committed;
} T1ScheduledAnimation;

T1ScheduledAnimation * T1_scheduled_animations_request_next(
    bool32_t endpoints_not_deltas);

void T1_scheduled_animations_commit(T1ScheduledAnimation * to_commit);

void T1_scheduled_animations_request_evaporate_and_destroy(
    const int32_t object_id,
    const uint64_t duration_us);

void T1_scheduled_animations_request_shatter_and_destroy(
    const int32_t object_id,
    const uint64_t duration_us);

void T1_scheduled_animations_request_fade_and_destroy(
    const int32_t object_id,
    const uint64_t duration_us);

void T1_scheduled_animations_request_fade_to(
    const int32_t zsprite_id,
    const uint64_t duration_us,
    const float target_alpha);

void T1_scheduled_animations_request_dud_dance(
    const int32_t object_id,
    const float magnitude);

void T1_scheduled_animations_request_bump(
    const int32_t object_id,
    const uint32_t wait);

void T1_scheduled_animations_delete_all(void);
void T1_scheduled_animations_delete_endpoint_anims_targeting(
    const int32_t object_id);
void T1_scheduled_animations_delete_all_anims_targeting(const int32_t object_id);

void T1_scheduled_animations_set_ignore_camera_but_retain_screenspace_pos(
    const int32_t zsprite_id,
    const float new_ignore_camera);

#ifdef __cplusplus
}
#endif

#endif // SCHEDULED_ANIMATION_H

#endif // SCHEDULED_ANIMS_ACTIVE
