#include "scheduled_animations.h"

#define SCHEDULED_ANIMATIONS_ARRAYSIZE 500
ScheduledAnimation scheduled_animations[
    SCHEDULED_ANIMATIONS_ARRAYSIZE];
uint32_t scheduled_animations_size = 0;

void construct_scheduled_animation(
    ScheduledAnimation * to_construct)
{
    to_construct->affected_object_id = 0;
    to_construct->affects_camera_not_object = false;
    to_construct->delta_x_per_second = 0.0f;
    to_construct->delta_y_per_second = 0.0f;
    to_construct->delta_z_per_second = 0.0f;
    to_construct->delta_z_per_second = 0.0f;
    to_construct->x_rotation_per_second = 0.0f;
    to_construct->y_rotation_per_second = 0.0f;
    to_construct->z_rotation_per_second = 0.0f;
    to_construct->scalefactor_delta_per_second = 0.0f;
    for (uint32_t i = 0; i < 4; i++) {
        to_construct->rgba_delta_per_second[i] = 0.0f;
    }
    to_construct->wait_first_microseconds = 0;
    to_construct->remaining_microseconds = 1000000;
    to_construct->delete_object_when_finished = false;
    to_construct->deleted = false;
}

void request_scheduled_animation(ScheduledAnimation * to_add)
{
    assert(to_add != NULL);
    
    if (to_add->remaining_microseconds == 0) {
        printf(
            "ERROR: You can't schedule an animation with a duration of 0 microseconds - please just apply the effect directly instead. Animation xy[%f,%f] rgba[%f,%f,%f,%f] zrot [%f]\n",
            to_add->delta_x_per_second,
            to_add->delta_y_per_second,
            to_add->rgba_delta_per_second[0],
            to_add->rgba_delta_per_second[1],
            to_add->rgba_delta_per_second[2],
            to_add->rgba_delta_per_second[3],
            to_add->z_rotation_per_second);
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
            return;
        }
    }
    
    assert(
        SCHEDULED_ANIMATIONS_ARRAYSIZE
            > scheduled_animations_size);
    
    scheduled_animations[scheduled_animations_size] = *to_add;
    scheduled_animations_size += 1;
}

void request_fade_and_destroy(
    const uint32_t object_id,
    const uint64_t duration_microseconds)
{
    // get current alpha
    // we'll go with the biggest diff found in case of
    // multiple objs
    float target_alpha = 0.0f;
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
            
            if (new_dist > cur_dist)
            {
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
    construct_scheduled_animation(&modify_alpha);
    modify_alpha.affected_object_id = object_id;
    modify_alpha.remaining_microseconds = duration_microseconds;
    modify_alpha.rgba_delta_per_second[3] = change_per_second;
    modify_alpha.delete_object_when_finished = true;
    request_scheduled_animation(&modify_alpha);
}

void request_fade_to(
    const uint32_t object_id,
    const uint64_t duration_microseconds,
    const float target_alpha)
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
    construct_scheduled_animation(&modify_alpha);
    modify_alpha.affected_object_id = object_id;
    modify_alpha.remaining_microseconds = duration_microseconds;
    modify_alpha.rgba_delta_per_second[3] = change_per_second;
    request_scheduled_animation(&modify_alpha);
}

void resolve_animation_effects(
    uint64_t microseconds_elapsed)
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
            if (animation_i == scheduled_animations_size - 1) {
                scheduled_animations_size -= 1;
            }
            continue;
        }
        
        uint64_t actual_elapsed = microseconds_elapsed;
        
        if (anim->wait_first_microseconds > 0)
        {
            if (actual_elapsed >
                anim->wait_first_microseconds)
            {
                actual_elapsed -=
                    anim->wait_first_microseconds;
                anim->wait_first_microseconds = 0;
            } else {
                
                anim->wait_first_microseconds -=
                    actual_elapsed;
                continue;
            }
        }
        
        assert(anim->wait_first_microseconds == 0);
        
        actual_elapsed =
            anim->remaining_microseconds > actual_elapsed ?
                actual_elapsed :
                    anim->remaining_microseconds;
        
        // delete if duration expired
        if (anim->remaining_microseconds == 0)
        {
            anim->deleted = true;
            if (animation_i == scheduled_animations_size)
            {
                scheduled_animations_size -= 1;
            }
            
            if (anim->delete_object_when_finished) {
                for (
                    int32_t tq_i = texquads_to_render_size - 1;
                    tq_i >= 0;
                    tq_i--)
                {
                    if (
                        texquads_to_render[tq_i].object_id ==
                            anim->affected_object_id)
                    {
                        printf(
                            "deleting object at tq_i %i of %u\n",
                            tq_i,
                            texquads_to_render_size);
                        
                        texquads_to_render[tq_i].deleted = true;
                        texquads_to_render[tq_i].visible = false;
                        texquads_to_render[tq_i]
                            .texturearray_i = -1;
                        texquads_to_render[tq_i].texture_i = -1;
                        
                        if (tq_i == texquads_to_render_size - 1)
                        {
                            printf(
                                "tq_to_rndr_size reduced to %u\n",
                                texquads_to_render_size);
                            texquads_to_render_size--;
                        }
                    }
                }
            }
        }
        
        assert(actual_elapsed <= anim->remaining_microseconds);
        anim->remaining_microseconds -= actual_elapsed;
        
        // apply effects
        for (
            uint32_t tq_i = 0;
            tq_i < texquads_to_render_size;
            tq_i++)
        {
            if (
                texquads_to_render[tq_i].object_id ==
                    anim->affected_object_id &&
                !texquads_to_render[tq_i].deleted)
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

                texquads_to_render[tq_i].scale_factor +=
                    (anim->scalefactor_delta_per_second *
                        actual_elapsed) / 1000000;
                if (anim->scalefactor_delta_per_second != 0.0f) {
                    printf(
                        "texquads_to_render[%u].scale_factor now: %f\n",
                        tq_i,
                        texquads_to_render[tq_i].scale_factor);
                }
                
                for (
                    uint32_t c = 0;
                    c < 4;
                    c++)
                {
                    texquads_to_render[tq_i].RGBA[c] +=
                        (anim->rgba_delta_per_second[c]
                            * actual_elapsed)
                                / 1000000;
                }
            }
        }
        
        // stuff
    }
}

void request_move_to(
    const uint32_t object_id,
    const uint64_t duration_microseconds,
    const bool32_t ignore_target_mid_x,
    const float target_mid_x,
    const bool32_t ignore_target_mid_y,
    const float target_mid_y)
{
    assert(duration_microseconds > 0);
    
    float left_change_per_second;
    float top_change_per_second;
    
    for (
        uint32_t tq_i = 0;
        tq_i < texquads_to_render_size;
        tq_i++)
    {
        if (texquads_to_render[tq_i].object_id == object_id)
        {
            float current_mid_x =
                texquads_to_render[tq_i].left_pixels
                    + (texquads_to_render[tq_i].width_pixels
                        * 0.5f);
            float current_mid_y =
                texquads_to_render[tq_i].top_pixels
                    - (texquads_to_render[tq_i].height_pixels
                        * 0.5f);
            
            float cur_x_dist =
                (current_mid_x - target_mid_x) *
                (current_mid_x - target_mid_x);
            float cur_y_dist =
                (current_mid_y - target_mid_y) *
                (current_mid_y - target_mid_y);
            
            // Find out how fast the left needs to change to reach
            // the target left exactly when the duration runs out
            left_change_per_second =
                ignore_target_mid_x ?
                    0.0f :
                (target_mid_x - current_mid_x) /
                    ((float)duration_microseconds / 1000000);
            top_change_per_second =
                ignore_target_mid_y ?
                    0.0f :
                    (target_mid_y - current_mid_y) /
                        ((float)duration_microseconds / 1000000);
            break;
        }
    }
    
    ScheduledAnimation move_request;
    construct_scheduled_animation(&move_request);
    move_request.affected_object_id = object_id;
    move_request.remaining_microseconds =
        duration_microseconds;
    move_request.delta_x_per_second =
        left_change_per_second;
    move_request.delta_y_per_second =
        top_change_per_second;
    request_scheduled_animation(&move_request);
}

void request_dud_dance(const uint32_t object_id)
{
    uint64_t step_size = 60000;
    for (
        uint64_t wait_first = 0;
        wait_first < step_size * 8;
        wait_first += step_size)
    {
        ScheduledAnimation move_request;
        construct_scheduled_animation(&move_request);
        move_request.affected_object_id = object_id;
        move_request.wait_first_microseconds = wait_first;
        move_request.remaining_microseconds = step_size;
        move_request.delta_x_per_second =
            wait_first % (step_size * 2) == 0 ?
                window_width * 0.07f
                : window_width * -0.07f;
        move_request.delta_y_per_second =
            wait_first % (step_size * 2) == 0 ?
                window_height * 0.07f
                : window_height * -0.07f;
        request_scheduled_animation(
            &move_request);
    }
}

void request_bump_animation(const uint32_t object_id)
{
    float duration = 200000;
    
    ScheduledAnimation embiggen_request;
    construct_scheduled_animation(&embiggen_request);
    embiggen_request.affected_object_id = object_id;
    embiggen_request.remaining_microseconds = duration * 0.5f;
    embiggen_request.scalefactor_delta_per_second = 1.5f;
    request_scheduled_animation(
        &embiggen_request);
    
    ScheduledAnimation revert_request;
    construct_scheduled_animation(&revert_request);
    revert_request.affected_object_id = object_id;
    revert_request.wait_first_microseconds = duration * 0.5f;
    revert_request.remaining_microseconds = duration * 0.5f;
    revert_request.scalefactor_delta_per_second = -1.5f;
    request_scheduled_animation(
        &revert_request);
}

