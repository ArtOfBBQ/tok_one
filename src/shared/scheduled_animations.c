#include "scheduled_animations.h"

bool32_t ignore_animation_effects = false;

#define SCHEDULED_ANIMATIONS_ARRAYSIZE 1000
ScheduledAnimation scheduled_animations[
    SCHEDULED_ANIMATIONS_ARRAYSIZE];
uint32_t scheduled_animations_size = 0;

void construct_scheduled_animation(
    ScheduledAnimation * to_construct)
{
    to_construct->affected_object_id = 0;

    to_construct->set_texture_array_i = false;
    to_construct->new_texture_array_i = -1;
    to_construct->set_texture_i = false;
    to_construct->new_texture_i = -1;
    
    to_construct->final_x_known = false;
    to_construct->final_y_known = false;
    to_construct->final_z_known = false;
    to_construct->delta_x_per_second = 0.0f;
    to_construct->delta_y_per_second = 0.0f;
    to_construct->delta_z_per_second = 0.0f;
    to_construct->delta_z_per_second = 0.0f;
    to_construct->x_rotation_per_second = 0.0f;
    to_construct->y_rotation_per_second = 0.0f;
    to_construct->z_rotation_per_second = 0.0f;

    to_construct->final_width_known = false;
    to_construct->final_height_known = false;
    to_construct->delta_width_per_second = 0.0f;
    to_construct->delta_height_per_second = 0.0f;
    
    to_construct->final_xscale_known = false;
    to_construct->final_yscale_known = false;
    to_construct->delta_xscale_per_second = 0.0f;
    to_construct->delta_yscale_per_second = 0.0f;
    for (uint32_t i = 0; i < 4; i++) {
        to_construct->rgba_delta_per_second[i] = 0.0f;
    }
    to_construct->wait_before_each_run = 0;
    to_construct->remaining_wait_before_next_run = 0;
    to_construct->duration_microseconds = 1000000;
    to_construct->runs = 1;
    to_construct->delete_object_when_finished = false;
    to_construct->deleted = false;
    to_construct->clientlogic_callback_when_finished_id = -1;
}

void request_scheduled_animation(
    ScheduledAnimation * to_add)
{
    log_assert(to_add != NULL);
    to_add->remaining_microseconds = to_add->duration_microseconds;
    
    log_assert(scheduled_animations_size < SCHEDULED_ANIMATIONS_ARRAYSIZE);
    for (
        int32_t i = 0;
        i < (int32_t)scheduled_animations_size;
        i++)
    {
        if (scheduled_animations[i].deleted)
        {
            scheduled_animations[i] = *to_add;
            return;
        }
    }
    
    log_assert(SCHEDULED_ANIMATIONS_ARRAYSIZE > scheduled_animations_size);
    
    scheduled_animations[scheduled_animations_size] = *to_add;
    scheduled_animations_size += 1;
    log_assert(
        SCHEDULED_ANIMATIONS_ARRAYSIZE
            > scheduled_animations_size);
}

void request_fade_and_destroy(
    const uint32_t object_id,
    const uint64_t wait_before_first_run,
    const uint64_t duration_microseconds)
{
    log_assert(duration_microseconds > 0);
    
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
    modify_alpha.remaining_wait_before_next_run = wait_before_first_run;
    modify_alpha.duration_microseconds = duration_microseconds;
    modify_alpha.rgba_delta_per_second[3] = change_per_second;
    modify_alpha.delete_object_when_finished = true;
    request_scheduled_animation(&modify_alpha);
}

void request_fade_to(
    const uint32_t object_id,
    const uint64_t wait_before_first_run,
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
    modify_alpha.remaining_wait_before_next_run = wait_before_first_run;
    modify_alpha.duration_microseconds = duration_microseconds;
    modify_alpha.rgba_delta_per_second[3] = change_per_second;
    request_scheduled_animation(&modify_alpha);
}

void resolve_animation_effects(uint64_t microseconds_elapsed)
{
    log_assert(microseconds_elapsed < 500000);
    
    ScheduledAnimation * anim;
    for (
        int32_t animation_i = (int32_t)(scheduled_animations_size - 1);
        animation_i >= 0;
        animation_i--)
    {
        // pointer for abbreviation
        anim = &scheduled_animations[animation_i];
        
        if (anim->deleted) {
            if (animation_i == (int32_t)scheduled_animations_size - 1) {
                scheduled_animations_size -= 1;
            }
            continue;
        }
        
        uint64_t actual_elapsed = microseconds_elapsed;
        
        if (anim->remaining_wait_before_next_run > 0) {
            if (actual_elapsed > anim->remaining_wait_before_next_run) {
                actual_elapsed -= anim->remaining_wait_before_next_run;
                anim->remaining_wait_before_next_run = 0;
            } else {
                anim->remaining_wait_before_next_run -= actual_elapsed;
                continue;
            }
        }
        
        log_assert(anim->remaining_wait_before_next_run == 0);
        
        actual_elapsed = anim->remaining_microseconds > actual_elapsed ?
            actual_elapsed : anim->remaining_microseconds;
        
        // delete if duration expired
        if (anim->remaining_microseconds == 0) {
            if (anim->runs > 1 || anim->runs == 0) {
                if (anim->runs > 1) {
                    anim->runs -= 1;
                } else {
                    log_assert(anim->runs == 0);
                }
                anim->remaining_microseconds = anim->duration_microseconds;
                anim->remaining_wait_before_next_run =
                    anim->wait_before_each_run - actual_elapsed;
                continue;
            }
            
            anim->deleted = true;
            if (animation_i == (int32_t)scheduled_animations_size) {
                scheduled_animations_size -= 1;
            }
            
            if (anim->clientlogic_callback_when_finished_id >= 0)  {
                client_logic_animation_callback(
                    anim->clientlogic_callback_when_finished_id);
            }
            
            if (anim->delete_object_when_finished) {
                for (
                    int32_t tq_i = (int32_t)texquads_to_render_size - 1;
                    tq_i >= 0;
                    tq_i--)
                {
                    if (
                        texquads_to_render[tq_i].object_id ==
                            anim->affected_object_id)
                    {
                        texquads_to_render[tq_i].deleted = true;
                        texquads_to_render[tq_i].visible = false;
                        texquads_to_render[tq_i].texturearray_i = -1;
                        texquads_to_render[tq_i].texture_i = -1;
                    }
                }
                
                for (
                    int32_t l_i = (int32_t)zlights_to_apply_size - 1;
                    l_i >= 0;
                    l_i--)
                {
                    if (
                        zlights_to_apply[l_i].object_id ==
                            anim->affected_object_id)
                    {
                        zlights_to_apply[l_i].deleted = true;
                        
                        if (l_i == (int32_t)zlights_to_apply_size - 1)
                        {
                            zlights_to_apply_size--;
                        }
                    }
                }
            }
        }
        
        if (actual_elapsed < 1) { continue; }
        
        log_assert(actual_elapsed <= anim->remaining_microseconds);
        anim->remaining_microseconds -= actual_elapsed;
        
        // apply effects
        for (
            int32_t l_i = (int32_t)zlights_to_apply_size - 1;
            l_i >= 0;
            l_i--)
        {
            if (
                zlights_to_apply[l_i].object_id ==
                    anim->affected_object_id &&
                !zlights_to_apply[l_i].deleted)
            {
                if (!anim->final_x_known) {
                    zlights_to_apply[l_i].x +=
                        (anim->delta_x_per_second * actual_elapsed) / 1000000;
                } else {
                    float diff_x = anim->final_mid_x - zlights_to_apply[l_i].x;
                    zlights_to_apply[l_i].x +=
                        (diff_x
                            / (anim->remaining_microseconds
                                + actual_elapsed)
                            * actual_elapsed);
                }
                
                if (!anim->final_y_known) {
                    zlights_to_apply[l_i].y +=
                        ((anim->delta_y_per_second
                            * actual_elapsed)
                                / 1000000);
                } else {
                    float diff_y = anim->final_mid_y - zlights_to_apply[l_i].y;
                    zlights_to_apply[l_i].y +=
                        diff_y
                            / (anim->remaining_microseconds
                                + actual_elapsed)
                                    * actual_elapsed;
                }
                
                for (
                    uint32_t c = 0;
                    c < 4;
                    c++)
                {
                    zlights_to_apply[l_i].RGBA[c] +=
                        (anim->rgba_delta_per_second[c]
                            * actual_elapsed)
                                / 1000000;
                }
            }
        }
        
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
                if (anim->set_texture_array_i) {
                    texquads_to_render[tq_i].texturearray_i =
                        anim->new_texture_array_i;
                }
                if (anim->set_texture_i) {
                    texquads_to_render[tq_i].texture_i = anim->new_texture_i;
                }
                
                if (!anim->final_x_known) {
                    texquads_to_render[tq_i].left_pixels +=
                        (anim->delta_x_per_second
                        * actual_elapsed)
                            / 1000000;
                } else {
                    float cur_mid_x =
                        texquads_to_render[tq_i].left_pixels +
                            (texquads_to_render[tq_i].width_pixels
                                / 2);
                    float diff_x = anim->final_mid_x - cur_mid_x;
                    texquads_to_render[tq_i].left_pixels +=
                        (diff_x
                            / (anim->remaining_microseconds
                                + actual_elapsed)
                            * actual_elapsed);
                }
                
                if (!anim->final_y_known) {
                    texquads_to_render[tq_i].top_pixels +=
                        ((anim->delta_y_per_second
                            * actual_elapsed)
                                / 1000000);
                } else {
                    float cur_mid_y =
                      texquads_to_render[tq_i].top_pixels -
                        (texquads_to_render[tq_i].height_pixels
                            / 2);
                    float diff_y =
                        anim->final_mid_y - cur_mid_y;
                    texquads_to_render[tq_i].top_pixels +=
                        diff_y
                            / (anim->remaining_microseconds
                                + actual_elapsed)
                                    * actual_elapsed;
                }
                
                texquads_to_render[tq_i].z_angle +=
                    (anim->z_rotation_per_second
                        * actual_elapsed)
                            / 1000000;

                // ***
                // absolute scaling
                if (!anim->final_width_known) {
                    texquads_to_render[tq_i].width_pixels +=
                        ((anim->delta_width_per_second
                            * actual_elapsed)
                                / 1000000);
                } else {
                    float cur_width = texquads_to_render[tq_i].width_pixels;
                    float diff_width = anim->final_width - cur_width;
                    texquads_to_render[tq_i].width_pixels +=
                        diff_width
                            / (anim->remaining_microseconds + actual_elapsed)
                                * actual_elapsed;
                }
                if (!anim->final_height_known) {
                    texquads_to_render[tq_i].height_pixels +=
                        ((anim->delta_height_per_second
                            * actual_elapsed)
                                / 1000000);
                } else {
                    float cur_height = texquads_to_render[tq_i].height_pixels;
                    float diff_height = anim->final_height - cur_height;
                    texquads_to_render[tq_i].height_pixels +=
                        diff_height
                            / (anim->remaining_microseconds + actual_elapsed)
                                * actual_elapsed;
                }
                // ***
                
                // ***
                // relative scaling
                if (anim->final_xscale_known) {
                    float diff_x =
                        anim->final_xscale -
                            texquads_to_render[tq_i].
                                scale_factor_x;
                    texquads_to_render[tq_i].scale_factor_x +=
                        (diff_x
                            / (anim->remaining_microseconds
                                + actual_elapsed)
                            * actual_elapsed);
                } else {
                    texquads_to_render[tq_i].scale_factor_x +=
                        (anim->delta_xscale_per_second *
                            actual_elapsed) / 1000000;
                }
                
                if (anim->final_yscale_known) {
                    float diff_y =
                        anim->final_yscale -
                            texquads_to_render[tq_i]
                                .scale_factor_y;
                    texquads_to_render[tq_i].scale_factor_y +=
                        (diff_y
                            / (anim->remaining_microseconds
                                + actual_elapsed)
                            * actual_elapsed);
                } else {
                    texquads_to_render[tq_i].scale_factor_y +=
                        (anim->delta_yscale_per_second *
                            actual_elapsed) / 1000000;
                }
                // ***
                
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
    }
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
        move_request.remaining_wait_before_next_run = wait_first;
        move_request.duration_microseconds = step_size;
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
    uint64_t duration = 200000;
    
    ScheduledAnimation embiggen_request;
    construct_scheduled_animation(&embiggen_request);
    embiggen_request.affected_object_id = object_id;
    embiggen_request.duration_microseconds = duration / 2;
    embiggen_request.final_xscale_known = true;
    embiggen_request.final_xscale = 1.35f;
    embiggen_request.final_yscale_known = true;
    embiggen_request.final_yscale = 1.35f;
    request_scheduled_animation(&embiggen_request);
    
    ScheduledAnimation revert_request;
    construct_scheduled_animation(&revert_request);
    revert_request.affected_object_id = object_id;
    revert_request.remaining_wait_before_next_run = duration / 2;
    revert_request.duration_microseconds = duration / 2;
    revert_request.final_xscale_known = true;
    revert_request.final_xscale = 1.0f;
    revert_request.final_yscale_known = true;
    revert_request.final_yscale = 1.0f;
    request_scheduled_animation(&revert_request);
}

