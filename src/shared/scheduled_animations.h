/* Schedule "Set and forget" animations */

#ifndef SCHEDULED_ANIMATION_H
#define SCHEDULED_ANIMATION_H

#include "common.h"
#include "clientlogic.h"
#include "bitmap_renderer.h" // for texquads_to_render
#include "zpolygon.h"

extern bool32_t ignore_animation_effects;
void resolve_animation_effects(uint64_t microseconds_elapsed);

typedef struct ScheduledAnimation {
    /*
    Any texquads (2d) or zlights with this object_id will be affected by the
    animation. If you request changes to the Z (depth) attribute of an object,
    including 2D members, the 2D members will ignore those instructions and
    perform what they can.
    
    note: affected_object_id below 0 will have no effect and is useless
    */
    int32_t affected_object_id;
    
    bool32_t affects_camera_not_object;
    
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
    
    float x_rotation_per_second;
    float y_rotation_per_second;
    float z_rotation_per_second;
    
    // *** scaling animations
    bool32_t final_xscale_known;
    union {
        float delta_xscale_per_second;
        float final_xscale;
    };
    bool32_t final_yscale_known;
    union {
        float delta_yscale_per_second;
        float final_yscale;
    };
    // *** end of scaling animations
    
    float rgba_delta_per_second[4];
    uint64_t wait_first_microseconds; // wait, then perform all animation runs
    uint64_t wait_before_each_run_microseconds; // resets each run
    uint64_t remaining_wait_before_next_run;
    uint64_t duration_microseconds;   // duration at the start of each run
    uint64_t remaining_microseconds;  // remaining duration (this run)
    uint32_t runs; // 0 to repeat forever, 1 to run once, 2 to run & repeat 1x 
    bool32_t delete_object_when_finished;
    bool32_t deleted;
    // ****
    // set to -1 to not callback at all
    // if 0 or higher, client_logic_animation_callback()
    // will be called with this id as its callback
    int32_t clientlogic_callback_when_finished_id;
    // ****
} ScheduledAnimation;

void construct_scheduled_animation(
    ScheduledAnimation * to_construct);

void request_scheduled_animation(ScheduledAnimation * to_add);

void request_fade_and_destroy(
    const uint32_t object_id,
    const uint64_t wait_first_microseconds,
    const uint64_t duration_microseconds);

void request_fade_to(
    const uint32_t object_id,
    const uint64_t wait_first_microseconds,
    const uint64_t duration_microseconds,
    const float target_alpha);

void request_dud_dance(
    const uint32_t object_id);

void request_bump_animation(
    const uint32_t object_id);

#endif

