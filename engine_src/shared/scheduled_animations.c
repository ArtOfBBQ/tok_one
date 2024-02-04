#include "scheduled_animations.h"

ScheduledAnimation * scheduled_animations;
uint32_t scheduled_animations_size = 0;

void init_scheduled_animations(void) {
    scheduled_animations = (ScheduledAnimation *)malloc_from_unmanaged(
        sizeof(ScheduledAnimation) * SCHEDULED_ANIMATIONS_ARRAYSIZE);
}

static void construct_scheduled_animation(
    ScheduledAnimation * to_construct)
{
    memset(to_construct, 0, sizeof(ScheduledAnimation));
    
    to_construct->affected_object_id = -1;
    
    to_construct->runs = 1;
    to_construct->clientlogic_callback_when_finished_id = -1;
}

ScheduledAnimation * next_scheduled_animation(void) {
    log_assert(
        scheduled_animations_size < SCHEDULED_ANIMATIONS_ARRAYSIZE);
    ScheduledAnimation * return_value = NULL;
    
    for (
        int32_t i = 0;
        i < (int32_t)scheduled_animations_size;
        i++)
    {
        if (scheduled_animations[i].deleted)
        {
            return_value = &scheduled_animations[i];
        }
    }
    
    if (return_value == NULL) {
        log_assert(
            scheduled_animations_size + 1 < SCHEDULED_ANIMATIONS_ARRAYSIZE);
        return_value = &scheduled_animations[scheduled_animations_size++];
    }
    
    construct_scheduled_animation(return_value);
    
    return return_value;
}

void commit_scheduled_animation(ScheduledAnimation * to_commit) {
    log_assert(!to_commit->committed);
    
    to_commit->remaining_microseconds = to_commit->duration_microseconds;
    
    for (uint32_t col_i = 0; col_i < 4; col_i++) {
        if (to_commit->final_rgba_known[col_i]) {
            log_assert(to_commit->final_rgba[col_i] >= 0.0f);
            log_assert(to_commit->final_rgba[col_i] <= 1.0f);
        }
    }
    
    if (to_commit->remaining_wait_before_next_run < 1) {
        delete_conflicting_animations(to_commit);
    }
    
    to_commit->committed = true;
}

void delete_conflicting_animations(ScheduledAnimation * priority_anim)
{
    float float_threshold = 0.00001f;
    
    for (
        int32_t i = 0;
        i < (int32_t)scheduled_animations_size;
        i++)
    {
        // candidate for elimination when it conflicts
        ScheduledAnimation * candidate = &scheduled_animations[i];
        
        // never conflict with yourself
        if (priority_anim == candidate) { continue; }
        
        if (
            (candidate->remaining_wait_before_next_run > 0 &&
                !priority_anim->destroy_even_waiting_duplicates) ||
            candidate->deleted ||
            candidate->clientlogic_callback_when_finished_id
                != -1 ||
            (candidate->affected_object_id !=
                priority_anim->affected_object_id) ||
            candidate->runs != 1)
        {
            continue;
        }
        
        if (
            priority_anim->final_z_known &&
            candidate->final_z_known)
        {
            candidate->deleted = true;
        }
        
        if (
            (
            !priority_anim->final_z_known &&
            (
                priority_anim->delta_z_per_second < -float_threshold ||
                priority_anim->delta_z_per_second >  float_threshold
            )) &&
            (
            !candidate->final_z_known &&
            (
                candidate->delta_z_per_second < -float_threshold ||
                candidate->delta_z_per_second >  float_threshold
            )))
        {
            candidate->deleted = true;
        }
        
        if (
            priority_anim->final_x_known &&
            candidate->final_x_known)
        {
            candidate->deleted = true;
        }
        
        if (
            (
            !priority_anim->final_x_known &&
            (
                priority_anim->delta_x_per_second < -float_threshold ||
                priority_anim->delta_x_per_second >  float_threshold
            )) &&
            (
            !candidate->final_x_known &&
            (
                candidate->delta_x_per_second < -float_threshold ||
                candidate->delta_x_per_second >  float_threshold
            )))
        {
            candidate->deleted = true;
        }
        
        if (
            (
            !priority_anim->final_x_angle_known &&
            (
                priority_anim->x_rotation_per_second < -float_threshold ||
                priority_anim->x_rotation_per_second >  float_threshold
            )) &&
            (
            !candidate->final_x_angle_known &&
            (
                candidate->x_rotation_per_second < -float_threshold ||
                candidate->x_rotation_per_second >  float_threshold
            )))
        {
            candidate->deleted = true;
        }
        
        if (
            priority_anim->final_y_known &&
            candidate->final_y_known)
        {
            candidate->deleted = true;
        }
        
        if (
            (
            !priority_anim->final_y_known &&
            (
                priority_anim->delta_y_per_second < -float_threshold ||
                priority_anim->delta_y_per_second >  float_threshold
            )) &&
            (
            !candidate->final_y_known &&
            (
                candidate->delta_y_per_second < -float_threshold ||
                candidate->delta_y_per_second >  float_threshold
            )))
        {
            candidate->deleted = true;
        }
        
        if (
            priority_anim->final_y_angle_known &&
            candidate->final_y_angle_known)
        {
            candidate->deleted = true;
        }
        
        if (
            (
            !priority_anim->final_y_angle_known &&
            (
                priority_anim->y_rotation_per_second < -float_threshold ||
                priority_anim->y_rotation_per_second >  float_threshold
            )) &&
            (
            !candidate->final_y_angle_known &&
            (
                candidate->y_rotation_per_second < -float_threshold ||
                candidate->y_rotation_per_second >  float_threshold
            )))
        {
            candidate->deleted = true;
        }
    }
}

void request_shatter_and_destroy(
    const int32_t object_id,
    const uint64_t wait_before_first_run,
    const uint64_t duration_microseconds,
    const float exploding_distance_per_second,
    const float xyz_rotation_per_second[3],
    const float linear_distance_per_second,
    const float linear_direction[3])
{
    log_assert(duration_microseconds > 0);
    log_assert(object_id >= 0);
    
    for (
        uint32_t zp_i = 0;
        zp_i < zpolygons_to_render->size;
        zp_i++)
    {
        if (zpolygons_to_render->cpu_data[zp_i].deleted ||
            !zpolygons_to_render->cpu_data[zp_i].committed ||
            zpolygons_to_render->cpu_data[zp_i].object_id != object_id)
        {
            continue;
        }
        
        ShatterEffect * shatter = next_shatter_effect();
        shatter->zpolygon_to_shatter_cpu = zpolygons_to_render->cpu_data[zp_i];
        shatter->zpolygon_to_shatter_gpu = zpolygons_to_render->gpu_data[zp_i];
        shatter->zpolygon_to_shatter_material =
            zpolygons_to_render->gpu_materials[zp_i * MAX_MATERIALS_SIZE];
        shatter->wait_first = wait_before_first_run;
        shatter->longest_random_delay_before_launch =
            (duration_microseconds * 3) / 2;
        shatter->start_fade_out_at_elapsed = (duration_microseconds / 10) * 8;
        shatter->finish_fade_out_at_elapsed = duration_microseconds;
        shatter->linear_direction[0] = linear_direction[0];
        shatter->linear_direction[1] = linear_direction[1];
        shatter->linear_direction[2] = linear_direction[2];
        shatter->linear_distance_per_second = linear_distance_per_second;
        shatter->exploding_distance_per_second = exploding_distance_per_second;
        shatter->squared_distance_per_second = 0.0f;
        
        shatter->xyz_rotation_per_second[0] = xyz_rotation_per_second[0];
        shatter->xyz_rotation_per_second[1] = xyz_rotation_per_second[1];
        shatter->xyz_rotation_per_second[2] = xyz_rotation_per_second[2];
        
        shatter->rgb_bonus_per_second[0] = 0.75f;
        shatter->rgb_bonus_per_second[1] = 0.35f;
        shatter->rgb_bonus_per_second[2] = 0.35f;
        commit_shatter_effect(shatter);
        
        zpolygons_to_render->cpu_data[zp_i].deleted = true;
    }
}

void request_fade_and_destroy(
    const int32_t object_id,
    const uint64_t wait_before_first_run,
    const uint64_t duration_microseconds)
{
    log_assert(duration_microseconds > 0);
    log_assert(object_id >= 0);
    
    // register scheduled animation
    ScheduledAnimation * modify_alpha = next_scheduled_animation();
    modify_alpha->affected_object_id = object_id;
    modify_alpha->remaining_wait_before_next_run = wait_before_first_run;
    modify_alpha->duration_microseconds = duration_microseconds;
    modify_alpha->rgba_delta_per_second[1] = -0.05f;
    modify_alpha->rgba_delta_per_second[2] = -0.05f;
    modify_alpha->final_rgba_known[3] = true;
    modify_alpha->final_rgba[3] = 0.0f;
    modify_alpha->delete_object_when_finished = true;
    modify_alpha->destroy_even_waiting_duplicates = false;
    commit_scheduled_animation(modify_alpha);
}

void request_fade_to(
    const int32_t object_id,
    const uint64_t wait_before_first_run,
    const uint64_t duration_microseconds,
    const float target_alpha)
{
    log_assert(object_id >= 0);
    
    // register scheduled animation
    ScheduledAnimation * modify_alpha = next_scheduled_animation();
    modify_alpha->affected_object_id = object_id;
    modify_alpha->remaining_wait_before_next_run = wait_before_first_run;
    modify_alpha->duration_microseconds = duration_microseconds;
    modify_alpha->final_rgba_known[3] = true;
    modify_alpha->final_rgba[3] = target_alpha;
    commit_scheduled_animation(modify_alpha);
}

static void resolve_single_animation_effects(
    ScheduledAnimation * anim,
    const uint64_t elapsed_this_run,
    const uint64_t remaining_microseconds_at_start_of_run)
{
    log_assert(remaining_microseconds_at_start_of_run >= elapsed_this_run);
    
    if (anim->deleted) { return; }
    
    anim->internal_trigger_count += 1;
    
    float time_delta = (float)elapsed_this_run / 1000000.0f;
    
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
                    (anim->delta_x_per_second * time_delta);
            } else {
                float diff_x = anim->final_mid_x - zlights_to_apply[l_i].x;
                zlights_to_apply[l_i].x +=
                    diff_x /
                        ((float)remaining_microseconds_at_start_of_run /
                            elapsed_this_run);
            }
            
            if (!anim->final_y_known) {
                zlights_to_apply[l_i].y +=
                    (anim->delta_y_per_second * time_delta);
            } else {
                float diff_y = anim->final_mid_y - zlights_to_apply[l_i].y;
                zlights_to_apply[l_i].y +=
                    diff_y /
                        ((float)remaining_microseconds_at_start_of_run /
                            elapsed_this_run);
            }
            
            if (!anim->final_z_known) {
                zlights_to_apply[l_i].z +=
                    ((anim->delta_z_per_second
                        * elapsed_this_run)
                            / 1000000);
            } else {
                float diff_z = anim->final_mid_z - zlights_to_apply[l_i].z;
                zlights_to_apply[l_i].z +=
                    diff_z /
                        ((float)remaining_microseconds_at_start_of_run /
                            elapsed_this_run);
            }
            
            if (!anim->final_reach_known) {
                zlights_to_apply[l_i].reach +=
                    ((anim->delta_reach_per_second
                        * elapsed_this_run)
                            / 1000000);
            } else {
                float diff_reach = anim->final_reach -
                    zlights_to_apply[l_i].reach;
                
                zlights_to_apply[l_i].reach +=
                    diff_reach /
                        ((float)remaining_microseconds_at_start_of_run /
                            elapsed_this_run);
            }
            
            for (
                uint32_t c = 0;
                c < 4;
                c++)
            {
                if (!anim->final_rgba_known[c]) {
                    float delta =
                        (anim->rgba_delta_per_second[c]
                            * elapsed_this_run)
                                / 1000000;
                    
                    if (delta > 0.0001f || delta < 0.0001f) {
                        zlights_to_apply[l_i].RGBA[c] += delta;
                    }
                } else {
                    float cur_val =
                        zlights_to_apply[l_i].RGBA[c];
                    float delta_val =
                        anim->final_rgba[c] - cur_val;
                    
                    if (delta_val > 0.0001f || delta_val < 0.0001f) {
                        zlights_to_apply[l_i].RGBA[c] +=
                            delta_val /
                                ((float)remaining_microseconds_at_start_of_run /
                                    elapsed_this_run);
                    }
                }
            }
        }
    }
    
    for (
        uint32_t zp_i = 0;
        zp_i < zpolygons_to_render->size;
        zp_i++)
    {
        if (
            zpolygons_to_render->cpu_data[zp_i].object_id !=
                anim->affected_object_id ||
            zpolygons_to_render->cpu_data[zp_i].deleted)
        {
            continue;
        }
        
        if (!anim->final_x_known) {
            zpolygons_to_render->gpu_data[zp_i].xyz[0] +=
                ((anim->delta_x_per_second *
                    (float)elapsed_this_run)
                            / 1000000.0f);
        } else {
            float diff_x = anim->final_mid_x -
                zpolygons_to_render->gpu_data[zp_i].xyz[0];
            zpolygons_to_render->gpu_data[zp_i].xyz[0] +=
                diff_x /
                    ((float)remaining_microseconds_at_start_of_run /
                        elapsed_this_run);
        }
        
        if (!anim->final_y_known) {
            zpolygons_to_render->gpu_data[zp_i].xyz[1] +=
                ((anim->delta_y_per_second *
                    (float)elapsed_this_run)
                        / 1000000.0f);
        } else {
            float diff_y = anim->final_mid_y -
                zpolygons_to_render->gpu_data[zp_i].xyz[1];
            zpolygons_to_render->gpu_data[zp_i].xyz[1] +=
                diff_y /
                    ((float)remaining_microseconds_at_start_of_run /
                        elapsed_this_run);
        }
        
        if (!anim->final_z_known) {
                zpolygons_to_render->gpu_data[zp_i].xyz[2] +=
                    ((anim->delta_z_per_second
                        * elapsed_this_run)
                            / 1000000);
        } else {
            float diff_z = anim->final_mid_z -
                zpolygons_to_render->gpu_data[zp_i].xyz[2];
            float pct_of_entire_run =
                (float)remaining_microseconds_at_start_of_run /
                    elapsed_this_run;
            
            zpolygons_to_render->gpu_data[zp_i].xyz[2] +=
                diff_z / pct_of_entire_run;
            if (remaining_microseconds_at_start_of_run == elapsed_this_run) {
                log_assert(
                    zpolygons_to_render->gpu_data[zp_i].xyz[2] ==
                        anim->final_mid_z);
            }
        }
        
        if (!anim->final_x_angle_known) {
            zpolygons_to_render->gpu_data[zp_i].xyz_angle[0] +=
                (anim->x_rotation_per_second
                    * elapsed_this_run)
                        / 1000000;
        } else {
            float diff_x_angle = anim->final_x_angle -
                zpolygons_to_render->gpu_data[zp_i].xyz_angle[0];
            zpolygons_to_render->gpu_data[zp_i].xyz_angle[0] +=
                diff_x_angle /
                    ((float)remaining_microseconds_at_start_of_run /
                        elapsed_this_run);
        }
        
        if (!anim->final_y_angle_known) {
            if (anim->y_rotation_per_second > 0.1f) {
                log_assert(1);
            }
            zpolygons_to_render->gpu_data[zp_i].xyz_angle[1] +=
                (anim->y_rotation_per_second
                    * elapsed_this_run)
                        / 1000000;
        } else {
            if (anim->y_rotation_per_second > 0.1f) {
                log_assert(1);
            }
            
            float diff_y_angle = anim->final_y_angle -
                zpolygons_to_render->gpu_data[zp_i].xyz_angle[1];
            zpolygons_to_render->gpu_data[zp_i].xyz_angle[1] +=
                diff_y_angle /
                    ((float)remaining_microseconds_at_start_of_run /
                        elapsed_this_run);
        }
        
        if (!anim->final_z_angle_known) {
            zpolygons_to_render->gpu_data[zp_i].xyz_angle[2] +=
                (anim->z_rotation_per_second
                    * elapsed_this_run)
                        / 1000000;
        } else {
            float diff_z_angle = anim->final_z_angle -
                zpolygons_to_render->gpu_data[zp_i].xyz_angle[2];
            zpolygons_to_render->gpu_data[zp_i].xyz_angle[2] +=
                diff_z_angle /
                    ((float)remaining_microseconds_at_start_of_run /
                        elapsed_this_run);
        }
        
        if (!anim->final_x_multiplier_known) {
            if (anim->delta_x_multiplier_per_second != 0.0f) {
                zpolygons_to_render->gpu_data[zp_i].xyz_multiplier[0] +=
                (anim->delta_x_multiplier_per_second
                    * elapsed_this_run)
                        / 1000000;
            }
        } else {
            float diff_x_multiplier = anim->final_x_multiplier -
                zpolygons_to_render->gpu_data[zp_i].xyz_multiplier[0];
            zpolygons_to_render->gpu_data[zp_i].xyz_multiplier[0] +=
                diff_x_multiplier /
                    ((float)remaining_microseconds_at_start_of_run /
                        elapsed_this_run);
        }
        
        if (!anim->final_y_multiplier_known) {
            if (anim->delta_y_multiplier_per_second != 0.0f) {
                zpolygons_to_render->gpu_data[zp_i].xyz_multiplier[1] +=
                (anim->delta_y_multiplier_per_second
                    * elapsed_this_run)
                        / 1000000;
            }
        } else {
            float diff_y_multiplier = anim->final_y_multiplier -
                zpolygons_to_render->gpu_data[zp_i].xyz_multiplier[1];
            zpolygons_to_render->gpu_data[zp_i].xyz_multiplier[1] +=
                diff_y_multiplier /
                    ((float)remaining_microseconds_at_start_of_run /
                        elapsed_this_run);
        }
        
        if (!anim->final_scale_known) {
            zpolygons_to_render->gpu_data[zp_i].scale_factor +=
                (anim->delta_scale_per_second *
                    elapsed_this_run) / 1000000;
        } else {
            float diff_scale =
                anim->final_scale - zpolygons_to_render->gpu_data[zp_i].
                    scale_factor;
            zpolygons_to_render->gpu_data[zp_i].scale_factor +=
                diff_scale
                    / ((float)remaining_microseconds_at_start_of_run
                        / elapsed_this_run);
        }
        
        if (anim->set_texture_array_i || anim->set_texture_i) {
            for (
                uint32_t mat_i = 0;
                mat_i < MAX_MATERIALS_SIZE;
                mat_i++)
            {
                if (anim->set_texture_array_i) {
                    zpolygons_to_render->gpu_materials[
                        (zp_i * MAX_MATERIALS_SIZE) + mat_i].texturearray_i =
                            anim->new_texture_array_i;
                }
                if (anim->set_texture_i) {
                    zpolygons_to_render->gpu_materials[
                        (zp_i * MAX_MATERIALS_SIZE) + mat_i].texture_i =
                            anim->new_texture_i;
                }
            }
        }
        
        for (
            uint32_t c = 0;
            c < 4;
            c++)
        {
            if (!anim->final_rgba_known[c]) {
                float delta = ((anim->rgba_delta_per_second[c]
                        * elapsed_this_run)
                    / 1000000);
                
                if (delta > 0.0001f || delta < 0.0001f) {
                    for (
                        uint32_t mat_i = 0;
                        mat_i < MAX_MATERIALS_SIZE;
                        mat_i++)
                    {
                        zpolygons_to_render->gpu_materials[
                            (zp_i * MAX_MATERIALS_SIZE) + mat_i].rgba[c] +=
                                delta;
                    }
                }
            } else {
                for (
                    uint32_t mat_i = 0;
                    mat_i < MAX_MATERIALS_SIZE;
                    mat_i++)
                {
                    float cur_val =
                        zpolygons_to_render->gpu_materials[
                            (zp_i * MAX_MATERIALS_SIZE) + mat_i].rgba[c];
                    float delta_val = anim->final_rgba[c] - cur_val;
                    
                    if (delta_val > 0.0001f || delta_val < 0.0001f) {
                        zpolygons_to_render->gpu_materials[
                            (zp_i * MAX_MATERIALS_SIZE) + mat_i].rgba[c] +=
                                (delta_val /
                                    ((float)
                                        remaining_microseconds_at_start_of_run /
                                            elapsed_this_run));
                    }
                }
            }
        }
        
        for (
            uint32_t c = 0;
            c < 3;
            c++)
        {
            if (!anim->final_rgb_bonus_known[c]) {
                float delta = ((anim->rgb_bonus_delta_per_second[c]
                        * elapsed_this_run)
                    / 1000000);
                    zpolygons_to_render->gpu_data[zp_i].bonus_rgb[c] +=
                        delta;
            } else {
                float cur_val = zpolygons_to_render->
                    gpu_data[zp_i].bonus_rgb[c];
                float delta_val = anim->final_rgb_bonus[c] - cur_val;
                
                zpolygons_to_render->gpu_data[zp_i].bonus_rgb[c] +=
                    delta_val /
                        ((float)remaining_microseconds_at_start_of_run /
                            elapsed_this_run);
            }
        }
    }    
}

void resolve_animation_effects(const uint64_t microseconds_elapsed) {
        
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
        
        if (!anim->committed) {
            continue;
        }
        
        uint64_t actual_elapsed = microseconds_elapsed;
        
        if (anim->remaining_wait_before_next_run > 0) {
            if (actual_elapsed >= anim->remaining_wait_before_next_run) {
                actual_elapsed -= anim->remaining_wait_before_next_run;
                anim->remaining_wait_before_next_run = 0;
                
                delete_conflicting_animations(anim);
            } else {
                anim->remaining_wait_before_next_run -= actual_elapsed;
                continue;
            }
        }
        
        uint64_t actual_elapsed_this_run = actual_elapsed;
        uint64_t remaining_microseconds_at_start_of_run =
            anim->remaining_microseconds;
        bool32_t delete_after_this_run = false;
        
        if (anim->remaining_microseconds > actual_elapsed) {
            anim->remaining_microseconds -= actual_elapsed;
        } else {
            
            actual_elapsed_this_run = anim->remaining_microseconds;
            
            // delete or set up next run
            if (anim->runs > 1 || anim->runs == 0) {
                
                if (anim->runs > 1) {
                    anim->runs -= 1;
                } else {
                    anim->runs = 0;
                }
                
                anim->remaining_wait_before_next_run =
                    anim->wait_before_each_run;
                
                anim->remaining_microseconds =
                    anim->duration_microseconds;
            } else {
                delete_after_this_run = true;
            }
        }
        
        if (actual_elapsed_this_run > 0) {
            resolve_single_animation_effects(
                anim,
                actual_elapsed_this_run,
                remaining_microseconds_at_start_of_run);
        }
        
        if (delete_after_this_run) {
            
            anim->deleted = true;
            if (animation_i == (int32_t)scheduled_animations_size) {
                scheduled_animations_size -= 1;
            }
            
            if (anim->clientlogic_callback_when_finished_id >= 0)  {
                client_logic_animation_callback(
                    anim->clientlogic_callback_when_finished_id,
                    anim->clientlogic_arg_1,
                    anim->clientlogic_arg_2,
                    anim->clientlogic_arg_3);
            }
            
            if (anim->delete_object_when_finished) {
                
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
                
                delete_zpolygon_object(anim->affected_object_id);
                
                delete_particle_effect(anim->affected_object_id);
            }
        }
    }
}

void request_dud_dance(
    const int32_t object_id,
    const float magnitude)
{
    uint64_t step_size = 70000;
    
    float delta = 0.07f * magnitude;
    
    for (
        uint32_t step = 0;
        step < 8;
        step++)
    {
        uint64_t wait_extra = step * step_size;
        
        ScheduledAnimation * move_request = next_scheduled_animation();
        move_request->affected_object_id = (int32_t)object_id;
        move_request->remaining_wait_before_next_run = wait_extra;
        move_request->duration_microseconds = step_size - 2000;
        move_request->delta_x_per_second = step % 2 == 0 ? delta : -delta;
        move_request->delta_y_per_second = move_request->delta_x_per_second;
        commit_scheduled_animation(move_request);
    }
}

void request_bump_animation(
    const int32_t object_id,
    const uint32_t wait)
{
    uint64_t duration = 150000;
    
    ScheduledAnimation * embiggen_request = next_scheduled_animation();
    embiggen_request->affected_object_id = (int32_t)object_id;
    embiggen_request->remaining_wait_before_next_run = wait;
    embiggen_request->duration_microseconds = duration / 5;
    embiggen_request->final_scale_known = true;
    embiggen_request->final_scale = 1.35f;
    commit_scheduled_animation(embiggen_request);
    
    ScheduledAnimation * revert_request = next_scheduled_animation();
    revert_request->affected_object_id = (int32_t)object_id;
    revert_request->remaining_wait_before_next_run = wait + duration / 2;
    revert_request->duration_microseconds = (duration / 5) * 4;
    revert_request->final_scale_known = true;
    revert_request->final_scale = 1.0f;
    commit_scheduled_animation(revert_request);
}

void delete_all_movement_animations_targeting(
    const int32_t object_id)
{
    for (uint32_t i = 0; i < scheduled_animations_size; i++) {
        if (scheduled_animations[i].affected_object_id == (int32_t)object_id &&
            (scheduled_animations[i].final_x_known ||
            scheduled_animations[i].final_y_known))
        {
            scheduled_animations[i].deleted = true;
        }
    }
}

void delete_all_rgba_animations_targeting(
    const int32_t object_id)
{
    for (uint32_t i = 0; i < scheduled_animations_size; i++) {
        if (scheduled_animations[i].affected_object_id == (int32_t)object_id &&
            (scheduled_animations[i].final_rgba_known[0] ||
             scheduled_animations[i].final_rgba_known[1] ||
             scheduled_animations[i].final_rgba_known[2] ||
             scheduled_animations[i].final_rgba_known[3]))
        {
            scheduled_animations[i].deleted = true;
        }
    }
}

void delete_all_animations_targeting(const int32_t object_id) {
    for (uint32_t i = 0; i < scheduled_animations_size; i++) {
        if (scheduled_animations[i].affected_object_id == object_id) {
            scheduled_animations[i].deleted = true;
        }
    }
}

void delete_all_repeatforever_animations(void) {
    for (uint32_t i = 0; i < scheduled_animations_size; i++) {
        if (scheduled_animations[i].runs == 0) {
            scheduled_animations[i].deleted = true;
        }
    }
}
