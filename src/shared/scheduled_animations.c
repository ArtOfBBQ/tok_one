#include "scheduled_animations.h"

#define scheduled_animations_arraysize 500
ScheduledAnimation scheduled_animations[scheduled_animations_arraysize];
uint32_t scheduled_animations_size = 0;

void request_scheduled_animation(ScheduledAnimation * to_add)
{
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
    
    scheduled_animations[scheduled_animations_size] = *to_add;
    scheduled_animations_size += 1;
}

void resolve_animation_effects(uint64_t microseconds_elapsed)
{
    ScheduledAnimation * anim;
    for (
        int32_t animation_i = scheduled_animations_size;
        animation_i >= 0;
        animation_i--)
    {
        // pointer for abbreviation
        anim = &scheduled_animations[animation_i];
        
        if (anim->deleted) {
            printf("removing a deleted animation...\n");
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

