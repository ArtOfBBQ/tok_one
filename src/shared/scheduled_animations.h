#ifndef SCHEDULED_ANIMATION_H
#define SCHEDULED_ANIMATION_H

#define SCHEDULED_ANIMATIONS_ARRAYSIZE 2000

#include "common.h"
#include "bitmap_renderer.h" // for texquads_to_render
#include "zpolygon.h"
#include "clientlogic.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_scheduled_animations(void);

void resolve_animation_effects(const uint64_t microseconds_elapsed);

typedef struct ScheduledAnimation {
    /*
    Any texquads (2d) or zlights with this object_id will be affected by the
    animation. If you request changes to the Z (depth) attribute of an object,
    including 2D members, the 2D members will ignore those instructions and
    perform what they can.
    
    note: affected_object_id below 0 will have no effect and is useless
    */
    int32_t affected_object_id;
    
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
    
    float x_rotation_per_second;
    float y_rotation_per_second;
    float z_rotation_per_second;
    
    // *** absolute scaling (change width/height)
    // acts as reach/diffuse modifier for lights since lights have no size
    union {
        bool32_t final_width_known;
        bool32_t final_reach_known;
    };
    union {
        float delta_width_per_second;
        float final_width;
        float delta_reach_per_second;
        float final_reach;
    };
    bool32_t final_height_known;
    union {
        float delta_height_per_second;
        float final_height;
    };
    
    // *** relative scaling animations
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
    
    bool32_t final_rgba_known[4];
    union {
        float rgba_delta_per_second[4];
        float final_rgba[4];
    };
    
    uint64_t wait_before_each_run;           // resets each run
    uint64_t remaining_wait_before_next_run; // gets reset by above each run
                                             // can be used as wait before 1st
                                             // run
    uint64_t duration_microseconds;   // duration at the start of each run
    uint64_t remaining_microseconds;  // remaining duration (this run)
    uint32_t runs; // 0 to repeat forever, 1 to run 1x, 2 to run 2x etc
    bool32_t delete_object_when_finished;
    bool32_t deleted;
    // ****
    // set to -1 to not callback at all
    // if 0 or higher, client_logic_animation_callback()
    // will be called with this id as its callback
    int32_t clientlogic_callback_when_finished_id;
    // ****
} ScheduledAnimation;

void construct_scheduled_animation(ScheduledAnimation * to_construct);

void request_scheduled_animation(ScheduledAnimation * to_add);

void request_fade_and_destroy(
    const int32_t object_id,
    const uint64_t wait_first_microseconds,
    const uint64_t duration_microseconds);

void request_fade_to(
    const int32_t object_id,
    const uint64_t wait_first_microseconds,
    const uint64_t duration_microseconds,
    const float target_alpha);

void request_dud_dance(const uint32_t object_id);

void request_bump_animation(
    const uint32_t object_id,
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
