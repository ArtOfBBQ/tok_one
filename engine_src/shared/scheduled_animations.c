#include "scheduled_animations.h"


ScheduledAnimationA * scheduled_animationAs;
uint32_t scheduled_animationAs_size = 0;

void init_scheduled_animations(void) {
    scheduled_animationAs = (ScheduledAnimationA *)malloc_from_unmanaged(
        sizeof(ScheduledAnimationA) * SCHEDULED_ANIMATIONS_ARRAYSIZE);
}

static void construct_scheduled_animationA(
    ScheduledAnimationA * to_construct,
    const bool32_t final_values_not_adds)
{
    memset(to_construct, 0, sizeof(ScheduledAnimationA));
    
    to_construct->affected_object_id = -1;
    
    if (final_values_not_adds) {
        to_construct->gpu_polygon_vals.xyz[0]       = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.xyz[1]       = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.xyz[2]       = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.xyz_angle[0] = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.xyz_angle[1] = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.xyz_angle[2] = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.bonus_rgb[0] = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.bonus_rgb[1] = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.bonus_rgb[2] = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.xyz_multiplier[0] =
            FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.xyz_multiplier[1] =
            FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.xyz_multiplier[2] =
            FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.xyz_offset[0] = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.xyz_offset[1] = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.xyz_offset[2] = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.scale_factor  = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.ignore_lighting =
            FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.ignore_camera =
            FLT_SCHEDULEDANIM_IGNORE;
        
        to_construct->gpu_polygon_vals.simd_padding[0] =
            FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.simd_padding[1] =
            FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.simd_padding[2] =
            FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.simd_padding[3] =
            FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.simd_padding[4] =
            FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_vals.simd_padding[5] =
            FLT_SCHEDULEDANIM_IGNORE;
        
        to_construct->gpu_polygon_material_vals.rgba[0] =
            FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_material_vals.rgba[1] =
            FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_material_vals.rgba[2] =
            FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_material_vals.rgba[3] =
            FLT_SCHEDULEDANIM_IGNORE;
        float scheduledanim_ignore = FLT_SCHEDULEDANIM_IGNORE;
        memcpy(
            &to_construct->gpu_polygon_material_vals.texture_i,
            &scheduledanim_ignore,
            4);
        memcpy(
            &to_construct->gpu_polygon_material_vals.texturearray_i,
            &scheduledanim_ignore,
            4);
        to_construct->gpu_polygon_material_vals.simd_padding[0] =
            FLT_SCHEDULEDANIM_IGNORE;
        to_construct->gpu_polygon_material_vals.simd_padding[1] =
            FLT_SCHEDULEDANIM_IGNORE;
        
        to_construct->lightsource_vals.flt_object_id = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->lightsource_vals.flt_deleted   = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->lightsource_vals.flt_committed = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->lightsource_vals.xyz[0]        = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->lightsource_vals.xyz[1]        = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->lightsource_vals.xyz[2]        = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->lightsource_vals.xyz_offset[0] = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->lightsource_vals.xyz_offset[1] = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->lightsource_vals.xyz_offset[2] = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->lightsource_vals.RGBA[0]       = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->lightsource_vals.RGBA[1]       = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->lightsource_vals.RGBA[2]       = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->lightsource_vals.RGBA[3]       = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->lightsource_vals.reach         = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->lightsource_vals.ambient       = FLT_SCHEDULEDANIM_IGNORE;
        to_construct->lightsource_vals.diffuse       = FLT_SCHEDULEDANIM_IGNORE;
    }
    
    to_construct->runs = 1;
    to_construct->final_values_not_adds = final_values_not_adds;
    
    to_construct->clientlogic_callback_when_finished_id = -1;
    to_construct->clientlogic_arg_1 = -1;
    to_construct->clientlogic_arg_2 = -1;
    to_construct->clientlogic_arg_3 = -1;
}

ScheduledAnimationA * next_scheduled_animationA(
    const bool32_t final_values_not_adds)
{
    log_assert(
        scheduled_animationAs_size < SCHEDULED_ANIMATIONS_ARRAYSIZE);
    ScheduledAnimationA * return_value = NULL;
    
    for (
        int32_t i = 0;
        i < (int32_t)scheduled_animationAs_size;
        i++)
    {
        if (scheduled_animationAs[i].deleted)
        {
            return_value = &scheduled_animationAs[i];
        }
    }
    
    if (return_value == NULL) {
        log_assert(
            scheduled_animationAs_size + 1 < SCHEDULED_ANIMATIONS_ARRAYSIZE);
        return_value = &scheduled_animationAs[scheduled_animationAs_size++];
    }
    
    construct_scheduled_animationA(return_value, final_values_not_adds);
    
    return return_value;
}

void commit_scheduled_animationA(ScheduledAnimationA * to_commit) {
    log_assert(!to_commit->committed);
    log_assert(to_commit->duration_microseconds > 0);
    log_assert(to_commit->remaining_microseconds == 0);
    
    to_commit->remaining_microseconds = to_commit->duration_microseconds;
    
    to_commit->committed = true;
}

void request_evaporate_and_destroy(
    const int32_t object_id,
    const uint64_t duration_microseconds)
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
        
        float duration_mod = (20000000.0f / (float)duration_microseconds);
        
        ParticleEffect * shatter_effect = next_particle_effect();
        shatter_effect->zpolygon_cpu = zpolygons_to_render->cpu_data[zp_i];
        shatter_effect->zpolygon_gpu = zpolygons_to_render->gpu_data[zp_i];
        shatter_effect->zpolygon_material =
            zpolygons_to_render->gpu_materials[zp_i * MAX_MATERIALS_SIZE];
        uint64_t shattered_verts_size =
            (uint64_t)all_mesh_summaries[shatter_effect->zpolygon_cpu.mesh_id].
                shattered_vertices_size;
        shatter_effect->particle_spawns_per_second = (uint32_t)(
            (shattered_verts_size * 1000000) /
                (uint64_t)(duration_microseconds + 1));
        shatter_effect->pause_between_spawns = 0;
        shatter_effect->vertices_per_particle = 3;
        shatter_effect->particle_lifespan = duration_microseconds;
        shatter_effect->use_shattered_mesh = true;
        
        float xy_dist   =  0.0020f;
        float z_dist    = -0.0045f;
        float xyz_angle =  0.0050f;
        float rgb_delta =  0.0050f;
        
        shatter_effect->random_textures_size = 0;
        
        shatter_effect->gpustats_pertime_random_add_1.xyz[0] = -xy_dist *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.xyz[1] = -xy_dist *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.xyz[2] = z_dist *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.xyz_angle[0] = xyz_angle *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.xyz_angle[1] = xyz_angle *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.xyz_angle[2] = xyz_angle *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.bonus_rgb[0] = rgb_delta *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.bonus_rgb[1] = rgb_delta *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.bonus_rgb[2] = rgb_delta *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_2.xyz[0] = xy_dist *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_2.xyz[1] = xy_dist *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_2.xyz[2] = z_dist *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_2.xyz_angle[0] =
            -xyz_angle * duration_mod;
        shatter_effect->gpustats_pertime_random_add_2.xyz_angle[1] =
            -xyz_angle * duration_mod;
        shatter_effect->gpustats_pertime_random_add_2.xyz_angle[2] =
            -xyz_angle * duration_mod;
        shatter_effect->gpustats_perexptime_add.scale_factor = -0.04f *
            duration_mod;
        
        shatter_effect->loops = 1;
        shatter_effect->generate_light = false;
        
        commit_particle_effect(shatter_effect);
        
        zpolygons_to_render->cpu_data[zp_i].deleted = true;
    }
}

void request_shatter_and_destroy(
    const int32_t object_id,
    const uint64_t duration_microseconds)
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
        
        float duration_mod = (20000000.0f / (float)duration_microseconds);
        
        ParticleEffect * shatter_effect = next_particle_effect();
        shatter_effect->zpolygon_cpu = zpolygons_to_render->cpu_data[zp_i];
        shatter_effect->zpolygon_gpu = zpolygons_to_render->gpu_data[zp_i];
        shatter_effect->zpolygon_material =
            zpolygons_to_render->gpu_materials[zp_i * MAX_MATERIALS_SIZE];
        uint64_t shattered_verts_size =
            (uint64_t)all_mesh_summaries[shatter_effect->zpolygon_cpu.mesh_id].
                shattered_vertices_size;
        shatter_effect->particle_spawns_per_second = (uint32_t)(
            (shattered_verts_size * 1000000) /
                (uint64_t)(duration_microseconds + 1));
        shatter_effect->pause_between_spawns = 0;
        shatter_effect->vertices_per_particle = 3;
        shatter_effect->particle_lifespan = duration_microseconds;
        shatter_effect->use_shattered_mesh = true;
        
        float xyz_dist = 0.02f;
        float xyz_angle = 0.05f;
        float rgb_delta = 0.05f;
        
        shatter_effect->random_textures_size = 0;
        
        shatter_effect->gpustats_pertime_random_add_1.xyz[0] = -xyz_dist *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.xyz[1] = -xyz_dist *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.xyz[2] = -xyz_dist *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.xyz_angle[0] = xyz_angle *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.xyz_angle[1] = xyz_angle *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.xyz_angle[2] = xyz_angle *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.bonus_rgb[0] = rgb_delta *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.bonus_rgb[1] = rgb_delta *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.bonus_rgb[2] = rgb_delta *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_2.xyz[0] = xyz_dist *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_2.xyz[1] = xyz_dist *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_2.xyz[2] = xyz_dist *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_2.xyz_angle[0] =
            -xyz_angle * duration_mod;
        shatter_effect->gpustats_pertime_random_add_2.xyz_angle[1] =
            -xyz_angle * duration_mod;
        shatter_effect->gpustats_pertime_random_add_2.xyz_angle[2] =
            -xyz_angle * duration_mod;
        shatter_effect->gpustats_perexptime_add.scale_factor = -0.07f *
            duration_mod;
        
        shatter_effect->loops = 1;
        shatter_effect->generate_light = false;
        
        commit_particle_effect(shatter_effect);
        
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
    ScheduledAnimationA * modify_alpha = next_scheduled_animationA(true);
    modify_alpha->affected_object_id = object_id;
    modify_alpha->remaining_wait_before_next_run = wait_before_first_run;
    modify_alpha->duration_microseconds = duration_microseconds;
    modify_alpha->lightsource_vals.reach = 0.0f;
    modify_alpha->gpu_polygon_material_vals.rgba[3] = 0.0f;
    modify_alpha->delete_object_when_finished = true;
    commit_scheduled_animationA(modify_alpha);
}

void request_fade_to(
    const int32_t object_id,
    const uint64_t wait_before_first_run,
    const uint64_t duration_microseconds,
    const float target_alpha)
{
    log_assert(object_id >= 0);
    
    // register scheduled animation
    ScheduledAnimationA * modify_alpha = next_scheduled_animationA(true);
    modify_alpha->affected_object_id = object_id;
    modify_alpha->remaining_wait_before_next_run = wait_before_first_run;
    modify_alpha->duration_microseconds = duration_microseconds;
    modify_alpha->gpu_polygon_material_vals.rgba[3] = target_alpha;
    commit_scheduled_animationA(modify_alpha);
}

void resolve_animationA_effects(const uint64_t microseconds_elapsed) {
    for (
        int32_t animation_i = (int32_t)scheduled_animationAs_size - 1;
        animation_i >= 0;
        animation_i--)
    {
        ScheduledAnimationA * anim = &scheduled_animationAs[animation_i];
        
        if (anim->deleted) {
            if (animation_i == (int32_t)scheduled_animationAs_size - 1) {
                scheduled_animationAs_size -= 1;
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
                
                // delete_conflicting_animationAs(anim);
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
        
        if (delete_after_this_run) {
            anim->deleted = true;
            if (animation_i == (int32_t)scheduled_animationAs_size) {
                scheduled_animationAs_size -= 1;
            }
        }
        
        // Apply effects
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
            
            float flt_actual_elapsed_this_run =
                (float)actual_elapsed_this_run;
            float flt_remaining_microseconds_this_run =
                (float)remaining_microseconds_at_start_of_run;
            
            SIMD_FLOAT simd_this_run_modifier =
                simd_div_floats(
                    simd_set_float(flt_actual_elapsed_this_run),
                    simd_set_float(flt_remaining_microseconds_this_run));
            
            float * anim_vals_ptr    =
                (float *)&anim->gpu_polygon_vals;
            float * target_vals_ptr =
                (float *)&zpolygons_to_render->gpu_data[zp_i];
            
            if (anim->final_values_not_adds) {
                float scheduled_anim_ignore_below = FLT_SCHEDULEDANIM_IGNORE;
                
                SIMD_FLOAT simd_scheduledanim_ignore_constant =
                    simd_set_float(scheduled_anim_ignore_below);
                
                log_assert((sizeof(GPUPolygon) / 4) % SIMD_FLOAT_LANES == 0);
                for (
                    uint32_t simd_step_i = 0;
                    (simd_step_i * sizeof(float)) < sizeof(GPUPolygon);
                    simd_step_i += SIMD_FLOAT_LANES)
                {
                    SIMD_FLOAT simd_anim_vals =
                        simd_load_floats((anim_vals_ptr + simd_step_i));
                    SIMD_FLOAT simd_target_vals =
                        simd_load_floats((target_vals_ptr + simd_step_i));
                    
                    SIMD_FLOAT simd_needed_to_goal = simd_sub_floats(
                        simd_anim_vals,
                        simd_target_vals);
                    SIMD_FLOAT simd_active_mask = simd_cmplt_floats(
                        simd_anim_vals,
                        simd_scheduledanim_ignore_constant);
                    
                    SIMD_FLOAT simd_addition =
                        simd_mul_floats(
                            simd_this_run_modifier,
                            simd_needed_to_goal);
                    
                    SIMD_FLOAT simd_masked_addition = simd_and_floats(
                        simd_addition,
                        simd_active_mask);
                    
                    SIMD_FLOAT simd_new_target_vals = simd_add_floats(
                        simd_target_vals,
                        simd_masked_addition);
                    
                    log_assert(
                        (target_vals_ptr + simd_step_i) <
                            (float *)&zpolygons_to_render[zp_i + 1].gpu_data);
                    simd_store_floats(
                        (target_vals_ptr + simd_step_i),
                        simd_new_target_vals);
                }
                #ifndef LOGGER_IGNORE_ASSERTS
                for (uint32_t i = 0; i < (sizeof(GPUPolygon) / 4); i++) {
                    // assert not NaN
                    log_assert((target_vals_ptr[i] == target_vals_ptr[i]));
                    log_assert(!isinf(target_vals_ptr[i]));
                    log_assert(!isnan(target_vals_ptr[i]));
                    log_assert((anim_vals_ptr[i] == anim_vals_ptr[i]));
                }
                #endif
                
                anim_vals_ptr    =
                    (float *)&anim->gpu_polygon_material_vals;
                uint32_t mat1_i = zp_i * MAX_MATERIALS_SIZE;
                for (
                    uint32_t mat_i = mat1_i;
                    mat_i < mat1_i + MAX_MATERIALS_SIZE;
                    mat_i++)
                {
                    target_vals_ptr =
                        (float *)&zpolygons_to_render->gpu_materials[mat_i];
                    
                    log_assert((sizeof(GPUPolygonMaterial) / 4) %
                        SIMD_FLOAT_LANES == 0);
                    for (
                        uint32_t simd_step_i = 0;
                        (simd_step_i * sizeof(float)) <
                            sizeof(GPUPolygonMaterial);
                        simd_step_i += SIMD_FLOAT_LANES)
                    {
                        SIMD_FLOAT simd_anim_vals =
                            simd_load_floats((anim_vals_ptr + simd_step_i));
                        SIMD_FLOAT simd_target_vals =
                            simd_load_floats((target_vals_ptr + simd_step_i));
                        
                        SIMD_FLOAT simd_needed_to_goal = simd_sub_floats(
                            simd_anim_vals,
                            simd_target_vals);
                        SIMD_FLOAT simd_active_mask = simd_cmplt_floats(
                            simd_anim_vals,
                            simd_scheduledanim_ignore_constant);
                        
                        SIMD_FLOAT simd_addition =
                            simd_mul_floats(
                                simd_this_run_modifier,
                                simd_needed_to_goal);
                        
                        SIMD_FLOAT simd_masked_addition = simd_and_floats(
                            simd_addition,
                            simd_active_mask);
                        
                        SIMD_FLOAT simd_new_target_vals = simd_add_floats(
                            simd_target_vals,
                            simd_masked_addition);
                        
                        log_assert(
                            (target_vals_ptr + simd_step_i) <
                                (float *)&zpolygons_to_render[zp_i + 1].gpu_data);
                        simd_store_floats(
                            (target_vals_ptr + simd_step_i),
                            simd_new_target_vals);
                    }
                }
            } else {
                log_assert((sizeof(GPUPolygon) / 4) % SIMD_FLOAT_LANES == 0);
                float flt_one_million = 1000000.0f;
                for (
                    uint32_t simd_step_i = 0;
                    (simd_step_i * sizeof(float)) < sizeof(GPUPolygon);
                    simd_step_i += SIMD_FLOAT_LANES)
                {
                    SIMD_FLOAT simd_anim_vals =
                        simd_load_floats((anim_vals_ptr + simd_step_i));
                    SIMD_FLOAT simd_target_vals =
                        simd_load_floats((target_vals_ptr + simd_step_i));
                    
                    SIMD_FLOAT simd_actual_elapsed =
                        simd_set_float(flt_actual_elapsed_this_run);
                    SIMD_FLOAT simd_one_million =
                        simd_set_float(flt_one_million);
                    
                    simd_target_vals = simd_add_floats(
                        simd_target_vals,
                        simd_div_floats(
                            simd_mul_floats(
                                simd_anim_vals,
                                simd_actual_elapsed),
                            simd_one_million));
                    
                    simd_store_floats(
                        (target_vals_ptr + simd_step_i),
                        simd_target_vals);
                }
                
                anim_vals_ptr = (float *)&anim->gpu_polygon_material_vals;
                uint32_t mat1_i = zp_i * MAX_MATERIALS_SIZE;
                for (
                    uint32_t mat_i = mat1_i;
                    mat_i < mat1_i + MAX_MATERIALS_SIZE;
                    mat_i++)
                {
                    target_vals_ptr =
                        (float *)&zpolygons_to_render->gpu_materials[mat_i];
                    
                    log_assert((sizeof(GPUPolygonMaterial) / 4) %
                        SIMD_FLOAT_LANES == 0);
                    for (
                        uint32_t simd_step_i = 0;
                        (simd_step_i * sizeof(float)) <
                            sizeof(GPUPolygonMaterial);
                        simd_step_i += SIMD_FLOAT_LANES)
                    {
                        SIMD_FLOAT simd_anim_vals =
                            simd_load_floats((anim_vals_ptr + simd_step_i));
                        SIMD_FLOAT simd_target_vals =
                            simd_load_floats((target_vals_ptr + simd_step_i));
                        
                        SIMD_FLOAT simd_actual_elapsed =
                            simd_set_float(flt_actual_elapsed_this_run);
                        SIMD_FLOAT simd_one_million =
                            simd_set_float(flt_one_million);
                        
                        simd_target_vals = simd_add_floats(
                            simd_target_vals,
                            simd_div_floats(
                                simd_mul_floats(
                                    simd_anim_vals,
                                    simd_actual_elapsed),
                                simd_one_million));
                        
                        simd_store_floats(
                            (target_vals_ptr + simd_step_i),
                            simd_target_vals);
                    }
                }
            }
        }
        
        for (
            uint32_t light_i = 0;
            light_i < zlights_to_apply_size;
            light_i++)
        {
            if (
                zlights_to_apply[light_i].object_id !=
                    anim->affected_object_id ||
                zlights_to_apply[light_i].deleted ||
                !zlights_to_apply[light_i].committed)
            {
                continue;
            }
            
            /*
                float flt_object_id;
                float flt_deleted;
                float flt_committed;
                float xyz[3];
                float xyz_offset[3];
                float RGBA[4];
                float reach;       // max distance before light intensity 0
                float ambient;     // how much ambient light does this radiate?
                float diffuse;     // how much diffuse light does this radiate?
            */
            if (anim->final_values_not_adds) {
                float this_run_modifier =
                    (float)actual_elapsed_this_run /
                        (float)remaining_microseconds_at_start_of_run;
                log_assert(this_run_modifier <= 1.0f);
                log_assert(this_run_modifier >  0.0f);
                
                float needed_to_goal;
                float active_mod;
                
                log_assert(
                    anim->lightsource_vals.flt_object_id ==
                        FLT_SCHEDULEDANIM_IGNORE);
                needed_to_goal = anim->lightsource_vals.flt_object_id -
                    zlights_to_apply[light_i].flt_object_id;
                active_mod = (anim->lightsource_vals.flt_object_id !=
                    FLT_SCHEDULEDANIM_IGNORE);
                zlights_to_apply[light_i].flt_object_id +=
                    ((this_run_modifier * needed_to_goal) *
                        active_mod);
                
                log_assert(
                    anim->lightsource_vals.flt_deleted ==
                        FLT_SCHEDULEDANIM_IGNORE);
                needed_to_goal = anim->lightsource_vals.flt_deleted -
                    zlights_to_apply[light_i].flt_deleted;
                active_mod = (anim->lightsource_vals.flt_deleted !=
                    FLT_SCHEDULEDANIM_IGNORE);
                zlights_to_apply[light_i].flt_deleted +=
                    ((this_run_modifier * needed_to_goal) *
                        active_mod);
                
                log_assert(
                    anim->lightsource_vals.flt_committed ==
                        FLT_SCHEDULEDANIM_IGNORE);
                needed_to_goal = anim->lightsource_vals.flt_committed -
                    zlights_to_apply[light_i].flt_committed;
                active_mod = (anim->lightsource_vals.flt_committed !=
                    FLT_SCHEDULEDANIM_IGNORE);
                zlights_to_apply[light_i].flt_committed +=
                    ((this_run_modifier * needed_to_goal) *
                        active_mod);
                
                needed_to_goal = anim->lightsource_vals.xyz[0] -
                    zlights_to_apply[light_i].xyz[0];
                active_mod = (anim->lightsource_vals.xyz[0] !=
                    FLT_SCHEDULEDANIM_IGNORE);
                zlights_to_apply[light_i].xyz[0] +=
                    ((this_run_modifier * needed_to_goal) *
                        active_mod);
                needed_to_goal = anim->lightsource_vals.xyz[1] -
                    zlights_to_apply[light_i].xyz[1];
                active_mod = (anim->lightsource_vals.xyz[1] !=
                    FLT_SCHEDULEDANIM_IGNORE);
                zlights_to_apply[light_i].xyz[1] +=
                    ((this_run_modifier * needed_to_goal) * active_mod);
                needed_to_goal = anim->lightsource_vals.xyz[2] -
                    zlights_to_apply[light_i].xyz[2];
                active_mod = (anim->lightsource_vals.xyz[2] !=
                    FLT_SCHEDULEDANIM_IGNORE);
                zlights_to_apply[light_i].xyz[2] +=
                    ((this_run_modifier * needed_to_goal) *
                        active_mod);
                
                needed_to_goal = anim->lightsource_vals.xyz_offset[0] -
                    zlights_to_apply[light_i].xyz_offset[0];
                active_mod = (anim->lightsource_vals.xyz_offset[0] !=
                    FLT_SCHEDULEDANIM_IGNORE);
                zlights_to_apply[light_i].xyz_offset[0] +=
                    ((this_run_modifier * needed_to_goal) *
                        active_mod);
                needed_to_goal = anim->lightsource_vals.xyz_offset[1] -
                    zlights_to_apply[light_i].xyz_offset[1];
                active_mod = (anim->lightsource_vals.xyz_offset[1] !=
                    FLT_SCHEDULEDANIM_IGNORE);
                zlights_to_apply[light_i].xyz_offset[1] +=
                    ((this_run_modifier * needed_to_goal) *
                        active_mod);
                needed_to_goal = anim->lightsource_vals.xyz_offset[2] -
                    zlights_to_apply[light_i].xyz_offset[2];
                active_mod = (anim->lightsource_vals.xyz_offset[2] !=
                    FLT_SCHEDULEDANIM_IGNORE);
                zlights_to_apply[light_i].xyz_offset[2] +=
                    ((this_run_modifier * needed_to_goal) *
                        active_mod);
                
                needed_to_goal = anim->lightsource_vals.RGBA[0] -
                    zlights_to_apply[light_i].RGBA[0];
                active_mod = (anim->lightsource_vals.RGBA[0] !=
                    FLT_SCHEDULEDANIM_IGNORE);
                zlights_to_apply[light_i].RGBA[0] +=
                    ((this_run_modifier * needed_to_goal) *
                        active_mod);
                needed_to_goal = anim->lightsource_vals.RGBA[1] -
                    zlights_to_apply[light_i].RGBA[1];
                active_mod = (anim->lightsource_vals.RGBA[1] !=
                    FLT_SCHEDULEDANIM_IGNORE);
                zlights_to_apply[light_i].RGBA[1] +=
                    ((this_run_modifier * needed_to_goal) *
                        active_mod);
                needed_to_goal = anim->lightsource_vals.RGBA[2] -
                    zlights_to_apply[light_i].RGBA[2];
                active_mod = (anim->lightsource_vals.RGBA[2] !=
                    FLT_SCHEDULEDANIM_IGNORE);
                zlights_to_apply[light_i].RGBA[2] +=
                    ((this_run_modifier * needed_to_goal) *
                        active_mod);
                needed_to_goal = anim->lightsource_vals.RGBA[3] -
                    zlights_to_apply[light_i].RGBA[3];
                active_mod = (anim->lightsource_vals.RGBA[3] !=
                    FLT_SCHEDULEDANIM_IGNORE);
                zlights_to_apply[light_i].RGBA[3] +=
                    ((this_run_modifier * needed_to_goal) *
                        active_mod);
                
                needed_to_goal = anim->lightsource_vals.reach -
                    zlights_to_apply[light_i].reach;
                active_mod = (anim->lightsource_vals.reach !=
                    FLT_SCHEDULEDANIM_IGNORE);
                zlights_to_apply[light_i].reach +=
                    ((this_run_modifier * needed_to_goal) *
                        active_mod);
                needed_to_goal = anim->lightsource_vals.ambient -
                    zlights_to_apply[light_i].ambient;
                active_mod = (anim->lightsource_vals.ambient !=
                    FLT_SCHEDULEDANIM_IGNORE);
                zlights_to_apply[light_i].ambient +=
                    ((this_run_modifier * needed_to_goal) *
                        active_mod);
                needed_to_goal = anim->lightsource_vals.diffuse -
                    zlights_to_apply[light_i].diffuse;
                active_mod = (anim->lightsource_vals.diffuse !=
                    FLT_SCHEDULEDANIM_IGNORE);
                zlights_to_apply[light_i].diffuse +=
                    ((this_run_modifier * needed_to_goal) *
                        active_mod);
            } else {
                log_assert(anim->lightsource_vals.flt_object_id == 0.0f);
                zlights_to_apply[light_i].flt_object_id +=
                    (anim->lightsource_vals.flt_object_id *
                        actual_elapsed_this_run) / 1000000.0f;
                log_assert(anim->lightsource_vals.flt_deleted == 0.0f);
                zlights_to_apply[light_i].flt_deleted +=
                    (anim->lightsource_vals.flt_deleted *
                        actual_elapsed_this_run) / 1000000.0f;
                log_assert(anim->lightsource_vals.flt_committed == 0.0f);
                zlights_to_apply[light_i].flt_committed +=
                    (anim->lightsource_vals.flt_committed *
                        actual_elapsed_this_run) / 1000000.0f;
                
                zlights_to_apply[light_i].xyz[0] +=
                    (anim->lightsource_vals.xyz[0] *
                        actual_elapsed_this_run) / 1000000.0f;
                zlights_to_apply[light_i].xyz[1] +=
                    (anim->lightsource_vals.xyz[1] *
                        actual_elapsed_this_run) / 1000000.0f;
                zlights_to_apply[light_i].xyz[2] +=
                    (anim->lightsource_vals.xyz[2] *
                        actual_elapsed_this_run) / 1000000.0f;
                
                zlights_to_apply[light_i].xyz_offset[0] +=
                    (anim->lightsource_vals.xyz_offset[0] *
                        actual_elapsed_this_run) / 1000000.0f;
                zlights_to_apply[light_i].xyz_offset[1] +=
                    (anim->lightsource_vals.xyz_offset[1] *
                        actual_elapsed_this_run) / 1000000.0f;
                zlights_to_apply[light_i].xyz_offset[2] +=
                    (anim->lightsource_vals.xyz_offset[2] *
                        actual_elapsed_this_run) / 1000000.0f;
                
                zlights_to_apply[light_i].RGBA[0] +=
                    (anim->lightsource_vals.RGBA[0] *
                        actual_elapsed_this_run) / 1000000.0f;
                zlights_to_apply[light_i].RGBA[1] +=
                    (anim->lightsource_vals.RGBA[1] *
                        actual_elapsed_this_run) / 1000000.0f;
                zlights_to_apply[light_i].RGBA[2] +=
                    (anim->lightsource_vals.RGBA[2] *
                        actual_elapsed_this_run) / 1000000.0f;
                zlights_to_apply[light_i].RGBA[3] +=
                    (anim->lightsource_vals.RGBA[3] *
                        actual_elapsed_this_run) / 1000000.0f;
                
                zlights_to_apply[light_i].reach +=
                    (anim->lightsource_vals.reach *
                        actual_elapsed_this_run) / 1000000.0f;
                zlights_to_apply[light_i].ambient +=
                    (anim->lightsource_vals.ambient *
                        actual_elapsed_this_run) / 1000000.0f;
                zlights_to_apply[light_i].diffuse +=
                    (anim->lightsource_vals.diffuse *
                        actual_elapsed_this_run) / 1000000.0f;
            }
        }
        
        if (delete_after_this_run) {
            anim->deleted = true;
            if (animation_i == (int32_t)scheduled_animationAs_size) {
                scheduled_animationAs_size -= 1;
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
        
        ScheduledAnimationA * move_request = next_scheduled_animationA(false);
        move_request->affected_object_id = (int32_t)object_id;
        move_request->remaining_wait_before_next_run = wait_extra;
        move_request->duration_microseconds = step_size - 2000;
        move_request->gpu_polygon_vals.xyz[0] = step % 2 == 0 ? delta : -delta;
        move_request->gpu_polygon_vals.xyz[1] =
            move_request->gpu_polygon_vals.xyz[0];
        commit_scheduled_animationA(move_request);
    }
}

void request_bump_animation(
    const int32_t object_id,
    const uint32_t wait)
{
    uint64_t duration = 150000;
    
    ScheduledAnimationA * embiggen_request = next_scheduled_animationA(true);
    embiggen_request->affected_object_id = (int32_t)object_id;
    embiggen_request->remaining_wait_before_next_run = wait;
    embiggen_request->duration_microseconds = duration / 5;
    embiggen_request->gpu_polygon_vals.scale_factor = 1.35f;
    commit_scheduled_animationA(embiggen_request);
    
    ScheduledAnimationA * revert_request = next_scheduled_animationA(true);
    revert_request->affected_object_id = (int32_t)object_id;
    revert_request->remaining_wait_before_next_run = wait + duration / 2;
    revert_request->duration_microseconds = (duration / 5) * 4;
    revert_request->gpu_polygon_vals.scale_factor = 1.0f;
    commit_scheduled_animationA(revert_request);
}

void delete_all_scheduled_animations(void)
{
    for (uint32_t i = 0; i < scheduled_animationAs_size; i++) {
        if (scheduled_animationAs[i].runs == 0) {
            scheduled_animationAs[i].deleted = true;
        }
    }
    scheduled_animationAs_size = 0;
}

void delete_all_movement_animations_targeting(
    const int32_t object_id)
{
    for (uint32_t i = 0; i < scheduled_animationAs_size; i++) {
        if (scheduled_animationAs[i].affected_object_id == (int32_t)object_id && (
            (((scheduled_animationAs[i].final_values_not_adds &&
            scheduled_animationAs[i].gpu_polygon_vals.xyz[0] !=
                FLT_SCHEDULEDANIM_IGNORE)  ||
            (scheduled_animationAs[i].gpu_polygon_vals.xyz[0] > 0.01f)))
            ||
            ((scheduled_animationAs[i].final_values_not_adds &&
            scheduled_animationAs[i].gpu_polygon_vals.xyz[1] !=
                FLT_SCHEDULEDANIM_IGNORE)  ||
            (scheduled_animationAs[i].gpu_polygon_vals.xyz[1] > 0.01f))))
        {
            scheduled_animationAs[i].deleted = true;
        }
    }
}

void delete_all_rgba_animations_targeting(
    const int32_t object_id)
{
    for (uint32_t i = 0; i < scheduled_animationAs_size; i++) {
        if (scheduled_animationAs[i].affected_object_id == (int32_t)object_id &&
            scheduled_animationAs[i].final_values_not_adds &&
            (scheduled_animationAs[i].gpu_polygon_material_vals.rgba[0] !=
                 FLT_SCHEDULEDANIM_IGNORE ||
             scheduled_animationAs[i].gpu_polygon_material_vals.rgba[1] !=
                 FLT_SCHEDULEDANIM_IGNORE ||
             scheduled_animationAs[i].gpu_polygon_material_vals.rgba[2] !=
                 FLT_SCHEDULEDANIM_IGNORE ||
             scheduled_animationAs[i].gpu_polygon_material_vals.rgba[3] !=
                 FLT_SCHEDULEDANIM_IGNORE))
        {
            scheduled_animationAs[i].deleted = true;
        }
    }
}

void delete_all_animations_targeting(const int32_t object_id) {
    for (uint32_t i = 0; i < scheduled_animationAs_size; i++) {
        if (scheduled_animationAs[i].affected_object_id == object_id) {
            scheduled_animationAs[i].deleted = true;
        }
    }
}

void delete_all_repeatforever_animations(void) {
    for (uint32_t i = 0; i < scheduled_animationAs_size; i++) {
        if (scheduled_animationAs[i].runs == 0) {
            scheduled_animationAs[i].deleted = true;
        }
    }
}
