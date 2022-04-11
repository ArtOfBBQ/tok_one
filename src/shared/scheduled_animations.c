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
                if (tq_i == 5) {
                    printf(
                        "delta_x_per_second: %f\n",
                        anim->delta_x_per_second);
                    printf(
                        "actual_elapsed: %u\n",
                        actual_elapsed);
                }
                float current_delta =
                    anim->delta_x_per_second * actual_elapsed;

                if (tq_i == 5) {
                    printf("current delta: %f\n", current_delta);
                }
                current_delta /= 1000000;

                if (tq_i == 5) {
                    printf("current delta divided: %f\n", current_delta);
                    printf("texquads_to_render[%u].x before: %f,",
                        tq_i,
                        texquads_to_render[tq_i].left);
                }
                texquads_to_render[tq_i].left +=
                    current_delta;
                
                if (tq_i == 5) {
                    printf("after: %f\n",
                        texquads_to_render[tq_i].left);
                }
            }
        }
    }
}

