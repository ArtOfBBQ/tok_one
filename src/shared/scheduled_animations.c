#include "scheduled_animations.h"

#define SCHEDULED_ANIMATIONS_ARRAYSIZE 500
ScheduledAnimation scheduled_animations[SCHEDULED_ANIMATIONS_ARRAYSIZE];
uint32_t scheduled_animations_size = 0;

void construct(ScheduledAnimation * to_construct) {
    to_construct->affected_object_id = 0;
    to_construct->affects_camera_not_object = false;
    to_construct->delta_x_per_second = 0.0f;
    to_construct->delta_y_per_second = 0.0f;
    to_construct->delta_z_per_second = 0.0f;
    to_construct->delta_z_per_second = 0.0f;
    to_construct->x_rotation_per_second = 0.0f;
    to_construct->y_rotation_per_second = 0.0f;
    to_construct->z_rotation_per_second = 0.0f;
    for (uint32_t i = 0; i < 4; i++) {
        to_construct->rgba_delta_per_second[i] = 0.0f;
    }
    to_construct->remaining_microseconds = 1000000.0f;
    to_construct->deleted = false;
}

void request_scheduled_animation(ScheduledAnimation * to_add)
{
    assert(to_add != NULL);
    
    if (to_add->remaining_microseconds == 0) {
        printf(
            "ERROR: You can't schedule an animation with a duration of 0 microseconds - please just apply the effect directly instead\n");
        assert(to_add->remaining_microseconds > 0);
    }
    assert(
        scheduled_animations_size
            < SCHEDULED_ANIMATIONS_ARRAYSIZE);
    
    for (
        int32_t i = 0;
        i < scheduled_animations_size;
        i++)
    {
        if (scheduled_animations[i].deleted)
        {
            scheduled_animations[i] = *to_add;
            scheduled_animations[i].deleted = false;
            return;
        }
    }
    
    assert(
        SCHEDULED_ANIMATIONS_ARRAYSIZE
            > scheduled_animations_size);
    scheduled_animations[scheduled_animations_size] = *to_add;
    scheduled_animations_size += 1;
}

void request_fade_to(
    uint32_t object_id,
    uint64_t duration_microseconds,
    float target_alpha)
{
    // get current alpha
    // we'll go with the biggest diff found in case of
    // multiple objs
    float current_alpha = target_alpha;
    
    for (
        uint32_t tq_i = 0;
        tq_i < texquads_to_render_size;
        tq_i++)
    {
        if (texquads_to_render[tq_i].object_id == object_id)
        {
            float cur_dist =
                (current_alpha - target_alpha) *
                (current_alpha - target_alpha);
            float new_dist =
                (texquads_to_render[tq_i].RGBA[3]
                    - target_alpha) *
                (texquads_to_render[tq_i].RGBA[3]
                    - target_alpha);
            if (new_dist > cur_dist) {
                current_alpha = texquads_to_render[tq_i].RGBA[3];
            }
        }
    }
    
    if (current_alpha == target_alpha) {
        return;
    }
    
    // Find out how fast the alpha needs to change to reach
    // the target alpha exactly when the duration runs out
    float change_per_second =
        (target_alpha - current_alpha) /
            ((float)duration_microseconds / 1000000);
    
    // register scheduled animation
    ScheduledAnimation modify_alpha;
    construct(&modify_alpha);
    modify_alpha.affected_object_id = object_id;
    modify_alpha.remaining_microseconds = duration_microseconds;
    modify_alpha.rgba_delta_per_second[3] = change_per_second;
    request_scheduled_animation(&modify_alpha);
}

void resolve_animation_effects(uint64_t microseconds_elapsed)
{
    ScheduledAnimation * anim;
    for (
        int32_t animation_i = (scheduled_animations_size - 1);
        animation_i >= 0;
        animation_i--)
    {
        // pointer for abbreviation
        anim = &scheduled_animations[animation_i];
        
        if (anim->deleted) {
            scheduled_animations_size -= 1;
            continue;
        }
       
        uint64_t actual_elapsed =
            anim->remaining_microseconds > microseconds_elapsed ?
                microseconds_elapsed :
                    anim->remaining_microseconds;

        // delete if duration expired
        assert(actual_elapsed <= anim->remaining_microseconds);
        anim->remaining_microseconds -= actual_elapsed;
        if (anim->remaining_microseconds == 0) {
            anim->deleted = true;
            if (animation_i == scheduled_animations_size) {
                scheduled_animations_size -= 1;
            }
        }
        
        // apply effects
        for (
            uint32_t tq_i = 0;
            tq_i < texquads_to_render_size;
            tq_i++)
        {
            if (
                texquads_to_render[tq_i].object_id ==
                    anim->affected_object_id)
            {
                texquads_to_render[tq_i].left_pixels +=
                    (anim->delta_x_per_second * actual_elapsed)
                        / 1000000;
                texquads_to_render[tq_i].top_pixels +=
                    (anim->delta_y_per_second * actual_elapsed)
                        / 1000000;
                texquads_to_render[tq_i].z_angle +=
                    (anim->z_rotation_per_second * actual_elapsed)
                        / 1000000;
                
                for (uint32_t c = 0; c < 4; c++) {
                    texquads_to_render[tq_i].RGBA[c] +=
                        (anim->rgba_delta_per_second[c]
                            * actual_elapsed)
                                / 1000000;
                }
            }
        }
    }
}

