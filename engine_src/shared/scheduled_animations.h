#ifndef SCHEDULED_ANIMATION_H
#define SCHEDULED_ANIMATION_H

#include "common.h"
#include "simd.h"

#include "zpolygon.h"
#include "particle.h"
// #include "clientlogic.h"

#define FLT_SCHEDULEDANIM_IGNORE 3.402823466e+38F
#ifdef __cplusplus
extern "C" {
#endif

void scheduled_animations_init(
    void (* arg_callback_function)(int32_t, float, float, int32_t));

float scheduled_animations_easing_revert(const float t);

void scheduled_animations_resolve(const uint64_t microseconds_elapsed);

typedef struct ScheduledAnimation {
    // Public:
    GPUPolygon         gpu_polygon_vals;
    // zPolygonCPU        zpolygon_cpu_vals;
    GPUPolygonMaterial gpu_polygon_material_vals;
    zLightSource       lightsource_vals;
    
    GPUPolygonMaterial onfinish_gpu_polygon_material_muls;
    GPUPolygonMaterial onfinish_gpu_polygon_material_adds;
    
    bool32_t           final_values_not_adds;
    
    uint64_t duration_microseconds;          // duration at the start of each
                                             // run
    
    uint64_t wait_before_each_run;
    uint64_t remaining_wait_before_next_run; // gets reset by above each run
                                             // can be used as wait before 1st
                                             // run
    int32_t affected_sprite_id;
    int32_t affected_touchable_id;
    uint32_t runs; // 0 to repeat forever, 1 to run 1x, 2 to run 2x etc
    bool32_t delete_object_when_finished;
    
    // ****
    // set to -1 to not callback at all
    // if 0 or higher, client_logic_animation_callback()
    // will be called with this id as its callback
    int32_t clientlogic_callback_when_finished_id;
    float clientlogic_arg_1;
    float clientlogic_arg_2;
    int32_t clientlogic_arg_3;
    // ****
    
    // Private:
    uint64_t remaining_microseconds;  // remaining duration (this run), private
    
    bool32_t delete_other_anims_targeting_same_object_id_on_commit;
    bool32_t deleted;
    bool32_t committed;
} ScheduledAnimation;

ScheduledAnimation * scheduled_animations_request_next(
    const bool32_t final_values_not_adds);

void scheduled_animations_commit(ScheduledAnimation * to_commit);

void scheduled_animations_request_evaporate_and_destroy(
    const int32_t object_id,
    const uint64_t duration_microseconds);

void scheduled_animations_request_shatter_and_destroy(
    const int32_t object_id,
    const uint64_t duration_microseconds);

void scheduled_animations_request_fade_and_destroy(
    const int32_t object_id,
    const uint64_t wait_first_microseconds,
    const uint64_t duration_microseconds);

void scheduled_animations_request_fade_to(
    const int32_t object_id,
    const uint64_t wait_first_microseconds,
    const uint64_t duration_microseconds,
    const float target_alpha);

void scheduled_animations_request_dud_dance(
    const int32_t object_id,
    const float magnitude);

void scheduled_animations_request_bump(
    const int32_t object_id,
    const uint32_t wait);

void scheduled_animations_delete_all(void);
void scheduled_animations_delete_movement_anims_targeting(const int32_t object_id);
void scheduled_animations_delete_rgba_anims_targeting(const int32_t object_id);
void scheduled_animations_delete_all_anims_targeting(const int32_t object_id);
void scheduled_animations_delete_all_repeatforever_anims(void);

#ifdef __cplusplus
}
#endif

#endif
