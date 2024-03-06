#ifndef SCHEDULED_ANIMATION_H
#define SCHEDULED_ANIMATION_H

#include "common.h"
#include "zpolygon.h"
#include "clientlogic.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_scheduled_animations(void);

void resolve_animation_effects(const uint64_t microseconds_elapsed);

typedef struct ScheduledAnimation {
    uint64_t wait_before_each_run;           // resets timer each run
    uint64_t remaining_wait_before_next_run; // gets reset by above each run
                                             // can be used as wait before 1st
                                             // run
    uint64_t duration_microseconds;   // duration at the start of each run
    uint64_t remaining_microseconds;  // remaining duration (this run)
    // uint64_t shatter_effect_duration; // 0 for no shatter effect
    
    /*
    Any texquads (2d) or zlights with this object_id will be affected by the
    animation. If you request changes to the Z (depth) attribute of an object,
    including 2D members, the 2D members will ignore those instructions and
    perform what they can.
    
    note: affected_object_id below 0 will have no effect and is useless
    */
    int32_t affected_object_id;
    
    // An animation, on activation, will delete any other animations that are
    // targeting the same properties of the same object ('Duplicates'). By
    // default, it skips deleting duplicates when
    bool32_t destroy_even_waiting_duplicates;
    
    bool32_t set_texture_array_i;
    int32_t new_texture_array_i;
    bool32_t set_texture_i;
    int32_t new_texture_i;
    
    // ******
    // TRANSLATION animations:
    // if you know the final x position at the end
    // of the animation, set final_x_known = true
    // and set final_mid_x to where you want to end up
    //
    // if not, set final_x_known = false
    // and set delta_x_per_second to move over a duration
    bool32_t final_x_known;
    union {
        float delta_x_per_second;
        float final_mid_x;
    };
    bool32_t final_y_known;
    union {
        float delta_y_per_second;
        float final_mid_y;
    };
    bool32_t final_z_known;
    union {
        float delta_z_per_second;
        float final_mid_z;
    };
    // ****** end of translation animations
    
    bool32_t final_x_angle_known;
    union {
        float x_rotation_per_second;
        float final_x_angle;
    };
    
    bool32_t final_y_angle_known;
    union {
        float y_rotation_per_second;
        float final_y_angle;
    };
    
    bool32_t final_z_angle_known;
    union {
        float z_rotation_per_second;
        float final_z_angle;
    };
    
    // *** absolute scaling (change width/height)
    // acts as reach/diffuse modifier for lights since lights have no size
    union {
        bool32_t final_x_multiplier_known;
        bool32_t final_reach_known;
    };
    union {
        float delta_x_multiplier_per_second;
        float final_x_multiplier;
        float delta_reach_per_second;
        float final_reach;
    };
    bool32_t final_y_multiplier_known;
    union {
        float delta_y_multiplier_per_second;
        float final_y_multiplier;
    };
    
    // *** relative scaling animations
    bool32_t final_scale_known;
    union {
        float delta_scale_per_second;
        float final_scale;
    };
    // *** end of scaling animations
    
    bool32_t final_rgba_known[4];
    union {
        float rgba_delta_per_second[4];
        float final_rgba[4];
    };
    
    bool32_t final_rgb_bonus_known[3];
    union {
        float rgb_bonus_delta_per_second[3];
        float final_rgb_bonus[3];
    };
    
    uint32_t internal_trigger_count; // TODO: remove me, I'm debug code
    uint32_t runs; // 0 to repeat forever, 1 to run 1x, 2 to run 2x etc
    bool32_t delete_object_when_finished;
    bool32_t deleted;
    bool32_t committed;
    
    // ****
    // set to -1 to not callback at all
    // if 0 or higher, client_logic_animation_callback()
    // will be called with this id as its callback
    int32_t clientlogic_callback_when_finished_id;
    float clientlogic_arg_1;
    float clientlogic_arg_2;
    int32_t clientlogic_arg_3;
    // ****
} ScheduledAnimation;

ScheduledAnimation * next_scheduled_animation(void);
void commit_scheduled_animation(ScheduledAnimation * to_commit);

void delete_conflicting_animations(ScheduledAnimation * priority_anim);

void request_shatter_and_destroy(
    const int32_t object_id,
    const uint64_t duration_microseconds);

void request_fade_and_destroy(
    const int32_t object_id,
    const uint64_t wait_first_microseconds,
    const uint64_t duration_microseconds);

void request_fade_to(
    const int32_t object_id,
    const uint64_t wait_first_microseconds,
    const uint64_t duration_microseconds,
    const float target_alpha);

void request_dud_dance(
    const int32_t object_id,
    const float magnitude);

void request_bump_animation(
    const int32_t object_id,
    const uint32_t wait);

void delete_all_movement_animations_targeting(const int32_t object_id);
void delete_all_rgba_animations_targeting(const int32_t object_id);
void delete_all_animations_targeting(const int32_t object_id);
void delete_all_repeatforever_animations(void);

extern ScheduledAnimation * scheduled_animations;
extern uint32_t scheduled_animations_size;

#ifdef __cplusplus
}
#endif

#endif
