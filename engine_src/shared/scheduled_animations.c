#include "scheduled_animations.h"

ScheduledAnimation * scheduled_animations;
uint32_t scheduled_animations_size = 0;

static uint32_t request_scheduled_anims_mutex_id = UINT32_MAX;

typedef struct PendingCallback {
    int32_t function_id;
    float arg_1;
    float arg_2;
    int32_t arg_3;
} PendingCallback;
#define MAX_PENDING_CALLBACKS 10
static PendingCallback pending_callbacks[MAX_PENDING_CALLBACKS];
static uint32_t pending_callbacks_size = 0;
static void (* callback_function)(int32_t, float, float, int32_t) = NULL;

void scheduled_animations_init(
    void (* arg_callback_function)(int32_t, float, float, int32_t))
{
    scheduled_animations = (ScheduledAnimation *)malloc_from_unmanaged(
        sizeof(ScheduledAnimation) * SCHEDULED_ANIMATIONS_ARRAYSIZE);
    common_memset_char(
        scheduled_animations,
        0,
        sizeof(ScheduledAnimation));
    scheduled_animations[0].deleted = true;
    
    for (uint32_t i = 1; i < SCHEDULED_ANIMATIONS_ARRAYSIZE; i++) {
        common_memcpy(
            &scheduled_animations[i],
            &scheduled_animations[0],
            sizeof(ScheduledAnimation));
        log_assert(scheduled_animations[i].deleted);
    }
    
    request_scheduled_anims_mutex_id = platform_init_mutex_and_return_id();
    
    callback_function = arg_callback_function;
}

static void construct_scheduled_animationA(
    ScheduledAnimation * to_construct,
    const bool32_t final_values_not_adds)
{
    common_memset_char(to_construct, 0, sizeof(ScheduledAnimation));
    log_assert(!to_construct->committed);
    
    to_construct->affected_object_id = -1;
    to_construct->affected_touchable_id = -1;
    
    if (final_values_not_adds) {
        common_memset_float(
            &to_construct->gpu_polygon_vals,
            FLT_SCHEDULEDANIM_IGNORE,
            sizeof(GPUPolygon));
        common_memset_float(
            &to_construct->gpu_polygon_material_vals,
            FLT_SCHEDULEDANIM_IGNORE,
            sizeof(GPUPolygonMaterial));
        common_memset_float(
            &to_construct->lightsource_vals,
            FLT_SCHEDULEDANIM_IGNORE,
            sizeof(zLightSource));
    }
    
    common_memset_float(
         &to_construct->onfinish_gpu_polygon_material_muls,
         1.0f,
         sizeof(GPUPolygonMaterial));
    
    to_construct->runs = 1;
    to_construct->final_values_not_adds = final_values_not_adds;
    
    to_construct->clientlogic_callback_when_finished_id = -1;
    to_construct->clientlogic_arg_1 = -1;
    to_construct->clientlogic_arg_2 = -1;
    to_construct->clientlogic_arg_3 = -1;
    
    log_assert(!to_construct->deleted);
    log_assert(!to_construct->committed);
}

ScheduledAnimation * next_scheduled_animation(
    const bool32_t final_values_not_adds)
{
    platform_mutex_lock(request_scheduled_anims_mutex_id);
    log_assert(
        scheduled_animations_size < SCHEDULED_ANIMATIONS_ARRAYSIZE);
    ScheduledAnimation * return_value = NULL;
    
    for (
        uint32_t i = 0;
        i < scheduled_animations_size;
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
        log_assert(return_value->deleted);
    }
    
    log_assert(return_value->deleted);
    construct_scheduled_animationA(return_value, final_values_not_adds);
    
    log_assert(!return_value->committed);
    log_assert(!return_value->deleted);
    
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
    
    return return_value;
}

void commit_scheduled_animation(ScheduledAnimation * to_commit) {
    platform_mutex_lock(request_scheduled_anims_mutex_id);
    log_assert(!to_commit->deleted);
    log_assert(!to_commit->committed);
    log_assert(to_commit->duration_microseconds > 0);
    log_assert(to_commit->remaining_microseconds == 0);
    
    if (to_commit->affected_object_id < 0) {
        if (to_commit->clientlogic_callback_when_finished_id < 0) {
            log_assert(to_commit->affected_touchable_id >= 0);
        }
    } else {
        log_assert(to_commit->affected_touchable_id == -1);
    }
    if (to_commit->affected_touchable_id < 0) {
        if (to_commit->clientlogic_callback_when_finished_id < 0) {
            log_assert(to_commit->affected_object_id >= 0);
        }
    } else {
        log_assert(to_commit->affected_object_id == -1);
    }
    
    to_commit->remaining_microseconds = to_commit->duration_microseconds;
    
    if (to_commit->delete_other_anims_targeting_same_object_id_on_commit) {
        for (uint32_t i = 0; i < scheduled_animations_size; i++) {
            if (
                scheduled_animations[i].affected_object_id >= 0 &&
                scheduled_animations[i].affected_object_id ==
                    to_commit->affected_object_id &&
                scheduled_animations[i].committed &&
                !scheduled_animations[i].deleted &&
                to_commit != &scheduled_animations[i])
            {
                scheduled_animations[i].deleted = true;
            }
        }
    }
    
    to_commit->committed = true;
    
    log_assert(to_commit->committed);
    log_assert(!to_commit->deleted);
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
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
        if (
            zpolygons_to_render->cpu_data[zp_i].deleted ||
            !zpolygons_to_render->cpu_data[zp_i].committed ||
            zpolygons_to_render->cpu_data[zp_i].object_id != object_id)
        {
            continue;
        }
        
        float duration_mod = (20000000.0f / (float)duration_microseconds);
        
        ParticleEffect * vaporize_effect = next_particle_effect();
        vaporize_effect->zpolygon_cpu = zpolygons_to_render->cpu_data[zp_i];
        vaporize_effect->zpolygon_gpu = zpolygons_to_render->gpu_data[zp_i];
        common_memcpy(
            vaporize_effect->zpolygon_materials,
            &zpolygons_to_render->gpu_materials[
                zp_i * MAX_MATERIALS_PER_POLYGON],
            sizeof(GPUPolygonMaterial) * MAX_MATERIALS_PER_POLYGON);
        
        uint64_t shattered_verts_size =
            (uint64_t)all_mesh_summaries[vaporize_effect->zpolygon_cpu.mesh_id].
                shattered_vertices_size;
        vaporize_effect->particle_spawns_per_second = (uint32_t)(
            (shattered_verts_size * 1000000) /
                (uint64_t)(duration_microseconds + 1));
        vaporize_effect->pause_between_spawns = 10;
        vaporize_effect->vertices_per_particle = 3;
        vaporize_effect->particle_lifespan = duration_microseconds;
        vaporize_effect->use_shattered_mesh = true;
        
        float xy_dist   =  0.0100f;
        float z_dist    = -0.0250f;
        float xyz_angle =  0.0100f;
        float rgb_delta =  0.0001f;
        
        vaporize_effect->random_textures_size = 0;
        
        vaporize_effect->gpustats_pertime_random_add_1.xyz[0] = -xy_dist *
            duration_mod;
        vaporize_effect->gpustats_pertime_random_add_1.xyz[1] = -xy_dist *
            duration_mod;
        vaporize_effect->gpustats_pertime_random_add_1.xyz[2] = z_dist *
            duration_mod;
        vaporize_effect->gpustats_pertime_random_add_1.xyz_angle[0] =
            xyz_angle * duration_mod;
        vaporize_effect->gpustats_pertime_random_add_1.xyz_angle[1] =
            xyz_angle * duration_mod;
        vaporize_effect->gpustats_pertime_random_add_1.xyz_angle[2] =
            xyz_angle * duration_mod;
        vaporize_effect->gpustats_pertime_random_add_1.bonus_rgb[0] =
            rgb_delta * duration_mod;
        vaporize_effect->gpustats_pertime_random_add_2.xyz[0] =
            xy_dist * duration_mod;
        vaporize_effect->gpustats_pertime_random_add_2.xyz[1] =
            xy_dist * duration_mod;
        vaporize_effect->gpustats_pertime_random_add_2.xyz[2] =
            z_dist * duration_mod;
        vaporize_effect->gpustats_pertime_random_add_2.xyz_angle[0] =
            -xyz_angle * duration_mod;
        vaporize_effect->gpustats_pertime_random_add_2.xyz_angle[1] =
            -xyz_angle * duration_mod;
        vaporize_effect->gpustats_pertime_random_add_2.xyz_angle[2] =
            -xyz_angle * duration_mod;
        vaporize_effect->gpustats_perexptime_add.scale_factor = -1.0f;
        
        vaporize_effect->loops = 1;
        vaporize_effect->generate_light = false;
        
        commit_particle_effect(vaporize_effect);
        
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
        common_memcpy(
            /* void * dst: */
                shatter_effect->zpolygon_materials,
            /* const void * src: */
                &zpolygons_to_render->gpu_materials[
                    zp_i * MAX_MATERIALS_PER_POLYGON],
            /* size_t n: */
                (sizeof(GPUPolygonMaterial) * MAX_MATERIALS_PER_POLYGON));
        
        #ifndef LOGGER_IGNORE_ASSERTS
        for (uint32_t mat_i = 0; mat_i < MAX_MATERIALS_PER_POLYGON; mat_i++) {
            log_assert(
                shatter_effect->zpolygon_materials[mat_i].rgba[0] ==
                    zpolygons_to_render->gpu_materials[
                        (zp_i * MAX_MATERIALS_PER_POLYGON)+mat_i].rgba[0]);
            log_assert(
                shatter_effect->zpolygon_materials[mat_i].rgba[1] ==
                    zpolygons_to_render->gpu_materials[
                        (zp_i * MAX_MATERIALS_PER_POLYGON)+mat_i].rgba[1]);
            log_assert(
                shatter_effect->zpolygon_materials[mat_i].texturearray_i ==
                    zpolygons_to_render->gpu_materials[
                        (zp_i * MAX_MATERIALS_PER_POLYGON)+mat_i].
                            texturearray_i);
            log_assert(
                shatter_effect->zpolygon_materials[mat_i].texture_i ==
                    zpolygons_to_render->gpu_materials[
                        (zp_i * MAX_MATERIALS_PER_POLYGON)+mat_i].
                            texture_i);
            log_assert(
                shatter_effect->zpolygon_materials[mat_i].diffuse ==
                    zpolygons_to_render->gpu_materials[
                        (zp_i * MAX_MATERIALS_PER_POLYGON)+mat_i].
                            diffuse);
            log_assert(
                shatter_effect->zpolygon_materials[mat_i].specular ==
                    zpolygons_to_render->gpu_materials[
                        (zp_i * MAX_MATERIALS_PER_POLYGON)+mat_i].
                            specular);
        }
        #endif
        
        uint64_t shattered_verts_size =
            (uint64_t)all_mesh_summaries[shatter_effect->zpolygon_cpu.mesh_id].
                shattered_vertices_size;
        log_assert(shattered_verts_size > 0);
        shatter_effect->particle_spawns_per_second = (uint32_t)(
            (shattered_verts_size * 1000000) /
                (uint64_t)(duration_microseconds + 1));
        shatter_effect->pause_between_spawns = 0;
        shatter_effect->vertices_per_particle = 6;
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
        
        log_assert(!shatter_effect->zpolygon_cpu.alpha_blending_enabled);
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
    
    // register scheduled animation
    ScheduledAnimation * fade_destroy = next_scheduled_animation(true);
    fade_destroy->affected_object_id = object_id;
    fade_destroy->remaining_wait_before_next_run = wait_before_first_run;
    fade_destroy->duration_microseconds = duration_microseconds;
    fade_destroy->lightsource_vals.reach = 0.0f;
    fade_destroy->gpu_polygon_material_vals.rgba[3] = 0.0f;
    fade_destroy->delete_object_when_finished = true;
    commit_scheduled_animation(fade_destroy);
}

void request_fade_to(
    const int32_t object_id,
    const uint64_t wait_before_first_run,
    const uint64_t duration_microseconds,
    const float target_alpha)
{
    log_assert(object_id >= 0);
    
    // register scheduled animation
    ScheduledAnimation * modify_alpha = next_scheduled_animation(true);
    modify_alpha->affected_object_id = object_id;
    modify_alpha->remaining_wait_before_next_run = wait_before_first_run;
    modify_alpha->duration_microseconds = duration_microseconds;
    modify_alpha->gpu_polygon_material_vals.rgba[3] = target_alpha;
    commit_scheduled_animation(modify_alpha);
}

void resolve_animation_effects(const uint64_t microseconds_elapsed) {
    platform_mutex_lock(request_scheduled_anims_mutex_id);
    pending_callbacks_size = 0;
    for (
        int32_t animation_i = (int32_t)scheduled_animations_size - 1;
        animation_i >= 0;
        animation_i--)
    {
        ScheduledAnimation * anim = &scheduled_animations[animation_i];
        
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
        bool32_t apply_muladds_this_frame = false;
        
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
            
            apply_muladds_this_frame = true;
        }
        
        if (delete_after_this_run) {
            anim->deleted = true;
            if (animation_i == (int32_t)scheduled_animations_size) {
                scheduled_animations_size -= 1;
            }
        }
        
        // Apply effects
        for (
            uint32_t zp_i = 0;
            zp_i < zpolygons_to_render->size;
            zp_i++)
        {
            if (
                (anim->affected_object_id >= 0 &&
                zpolygons_to_render->cpu_data[zp_i].object_id !=
                    anim->affected_object_id) ||
                (anim->affected_touchable_id >= 0 &&
                zpolygons_to_render->gpu_data[zp_i].touchable_id !=
                    anim->affected_touchable_id)
                ||
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
                    simd_set1_float(flt_actual_elapsed_this_run),
                    simd_set1_float(flt_remaining_microseconds_this_run));
            
            float * anim_vals_ptr    =
                (float *)&anim->gpu_polygon_vals;
            float * target_vals_ptr =
                (float *)&zpolygons_to_render->gpu_data[zp_i];
            
            if (anim->final_values_not_adds) {
                float scheduled_anim_ignore_below = FLT_SCHEDULEDANIM_IGNORE;
                
                SIMD_FLOAT simd_scheduledanim_ignore_constant =
                    simd_set1_float(scheduled_anim_ignore_below);
                
                log_assert((sizeof(GPUPolygon) / 4) % SIMD_FLOAT_LANES == 0);
                for (
                    uint32_t simd_step_i = 0;
                    ((simd_step_i+1) * sizeof(float)) < sizeof(GPUPolygon);
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
                for (uint32_t i = 0; i < (sizeof(GPUPolygon) / 4)-1; i++) {
                    // assert not NaN
                    if (target_vals_ptr[i] != 0) {
                        log_assert((target_vals_ptr[i] == target_vals_ptr[i]));
                        log_assert(!isinf(target_vals_ptr[i]));
                        log_assert(!isnan(target_vals_ptr[i]));
                    }
                }
                #endif
                
                anim_vals_ptr = (float *)&anim->gpu_polygon_material_vals;
                uint32_t mat1_i = zp_i * MAX_MATERIALS_PER_POLYGON;
                for (
                    uint32_t mat_i = mat1_i;
                    mat_i < mat1_i + MAX_MATERIALS_PER_POLYGON;
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
                        simd_set1_float(flt_actual_elapsed_this_run);
                    SIMD_FLOAT simd_one_million =
                        simd_set1_float(flt_one_million);
                    
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
                uint32_t mat1_i = zp_i * MAX_MATERIALS_PER_POLYGON;
                for (
                    uint32_t mat_i = mat1_i;
                    mat_i < mat1_i + MAX_MATERIALS_PER_POLYGON;
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
                            simd_set1_float(flt_actual_elapsed_this_run);
                        SIMD_FLOAT simd_one_million =
                            simd_set1_float(flt_one_million);
                        
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
                
                if (apply_muladds_this_frame) {
                    float * muls_ptr =
                        (float *)&anim->onfinish_gpu_polygon_material_muls;
                    float * adds_ptr =
                        (float *)&anim->onfinish_gpu_polygon_material_adds;
                    
                    mat1_i = zp_i * MAX_MATERIALS_PER_POLYGON;
                    for (
                        uint32_t mat_i = mat1_i;
                        mat_i < mat1_i + MAX_MATERIALS_PER_POLYGON;
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
                            SIMD_FLOAT simd_muls =
                                simd_load_floats((muls_ptr + simd_step_i));
                            SIMD_FLOAT simd_adds =
                                simd_load_floats((adds_ptr + simd_step_i));
                            
                            SIMD_FLOAT simd_target_vals =
                                simd_load_floats(
                                    (target_vals_ptr + simd_step_i));
                            
                            simd_target_vals = simd_add_floats(
                                    simd_mul_floats(
                                        simd_target_vals,
                                        simd_muls),
                                    simd_adds);
                            
                            simd_store_floats(
                                (target_vals_ptr + simd_step_i),
                                simd_target_vals);
                        }
                    }
                }
            }
        }
        
        log_assert((sizeof(zLightSource) / 4) % SIMD_FLOAT_LANES == 0);
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
            
            float flt_actual_elapsed_this_run =
                (float)actual_elapsed_this_run;
            float flt_remaining_microseconds_this_run =
                (float)remaining_microseconds_at_start_of_run;
            
            SIMD_FLOAT simd_this_run_modifier =
                simd_div_floats(
                    simd_set1_float(flt_actual_elapsed_this_run),
                    simd_set1_float(flt_remaining_microseconds_this_run));
            
            float * anim_vals_ptr = (float *)&anim->lightsource_vals;
            float * target_vals_ptr = (float *)&zlights_to_apply[light_i];
            
            if (anim->final_values_not_adds) {
                float scheduled_anim_ignore_below = FLT_SCHEDULEDANIM_IGNORE;
                
                SIMD_FLOAT simd_scheduledanim_ignore_constant =
                    simd_set1_float(scheduled_anim_ignore_below);
                
                log_assert((sizeof(zLightSource) / 4) % SIMD_FLOAT_LANES == 0);
                for (
                    uint32_t simd_step_i = 0;
                    (simd_step_i * sizeof(float)) < sizeof(zLightSource);
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
                    
                    SIMD_FLOAT simd_addition = simd_mul_floats(
                        simd_this_run_modifier,
                        simd_needed_to_goal);
                    
                    SIMD_FLOAT simd_masked_addition = simd_and_floats(
                        simd_addition,
                        simd_active_mask);
                    
                    SIMD_FLOAT simd_new_target_vals = simd_add_floats(
                        simd_target_vals,
                        simd_masked_addition);
                    
                    simd_store_floats(
                        (target_vals_ptr + simd_step_i),
                        simd_new_target_vals);
                }
                #ifndef LOGGER_IGNORE_ASSERTS
                for (uint32_t i = 0; i < (sizeof(zLightSource) / 4); i++) {
                    // assert not NaN
                    log_assert((target_vals_ptr[i] == target_vals_ptr[i]));
                    log_assert(!isinf(target_vals_ptr[i]));
                    log_assert(!isnan(target_vals_ptr[i]));
                    log_assert((anim_vals_ptr[i] == anim_vals_ptr[i]));
                }
                #endif
                
            } else {
                log_assert((sizeof(zLightSource) / 4) % SIMD_FLOAT_LANES == 0);
                float flt_one_million = 1000000.0f;
                for (
                    uint32_t simd_step_i = 0;
                    (simd_step_i * sizeof(float)) < sizeof(zLightSource);
                    simd_step_i += SIMD_FLOAT_LANES)
                {
                    SIMD_FLOAT simd_anim_vals =
                        simd_load_floats((anim_vals_ptr + simd_step_i));
                    SIMD_FLOAT simd_target_vals =
                        simd_load_floats((target_vals_ptr + simd_step_i));
                    
                    SIMD_FLOAT simd_actual_elapsed =
                        simd_set1_float(flt_actual_elapsed_this_run);
                    SIMD_FLOAT simd_one_million =
                        simd_set1_float(flt_one_million);
                    
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
        
        if (delete_after_this_run) {
            anim->deleted = true;
            if (animation_i == (int32_t)scheduled_animations_size) {
                scheduled_animations_size -= 1;
            }
            
            if (anim->clientlogic_callback_when_finished_id >= 0)  {
                // We run these later outside of the mutex
                log_assert(pending_callbacks_size + 1 <= MAX_PENDING_CALLBACKS);
                pending_callbacks[pending_callbacks_size].function_id =
                    anim->clientlogic_callback_when_finished_id;
                pending_callbacks[pending_callbacks_size].arg_1 =
                    anim->clientlogic_arg_1;
                pending_callbacks[pending_callbacks_size].arg_2 =
                    anim->clientlogic_arg_2;
                pending_callbacks[pending_callbacks_size].arg_3 =
                    anim->clientlogic_arg_3;
                pending_callbacks_size += 1;
            }
            
            if (anim->delete_object_when_finished) {
                delete_zlight(anim->affected_object_id);
                
                delete_zpolygon_object(anim->affected_object_id);
                
                delete_particle_effect(anim->affected_object_id);
            }
        }
    }
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
    
    for (uint32_t i = 0; i < pending_callbacks_size; i++) {
        callback_function(
            pending_callbacks[i].function_id,
            pending_callbacks[i].arg_1,
            pending_callbacks[i].arg_2,
            pending_callbacks[i].arg_3);
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
        
        ScheduledAnimation * move_request = next_scheduled_animation(false);
        move_request->affected_object_id = (int32_t)object_id;
        move_request->remaining_wait_before_next_run = wait_extra;
        move_request->duration_microseconds = step_size - 2000;
        move_request->gpu_polygon_vals.xyz[0] = step % 2 == 0 ? delta : -delta;
        move_request->gpu_polygon_vals.xyz[1] =
            move_request->gpu_polygon_vals.xyz[0];
        commit_scheduled_animation(move_request);
    }
}

void request_bump_animation(
    const int32_t object_id,
    const uint32_t wait)
{
    uint64_t duration = 150000;
    
    ScheduledAnimation * embiggen_request = next_scheduled_animation(true);
    embiggen_request->affected_object_id = (int32_t)object_id;
    embiggen_request->remaining_wait_before_next_run = wait;
    embiggen_request->duration_microseconds = duration / 5;
    embiggen_request->gpu_polygon_vals.scale_factor = 1.35f;
    commit_scheduled_animation(embiggen_request);
    
    ScheduledAnimation * revert_request = next_scheduled_animation(true);
    revert_request->affected_object_id = (int32_t)object_id;
    revert_request->remaining_wait_before_next_run = wait + duration / 2;
    revert_request->duration_microseconds = (duration / 5) * 4;
    revert_request->gpu_polygon_vals.scale_factor = 1.0f;
    commit_scheduled_animation(revert_request);
}

void delete_all_scheduled_animations(void)
{
    platform_mutex_lock(request_scheduled_anims_mutex_id);
    for (uint32_t i = 0; i < scheduled_animations_size; i++) {
        if (
            !scheduled_animations[i].deleted &&
            scheduled_animations[i].runs == 0)
        {
            scheduled_animations[i].deleted = true;
        }
    }
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
}

void delete_all_movement_animations_targeting(
    const int32_t object_id)
{
    platform_mutex_lock(request_scheduled_anims_mutex_id);
    for (uint32_t i = 0; i < scheduled_animations_size; i++) {
        if (
            !scheduled_animations[i].deleted &&
            scheduled_animations[i].affected_object_id == (int32_t)object_id &&
            (
                (((scheduled_animations[i].final_values_not_adds &&
                scheduled_animations[i].gpu_polygon_vals.xyz[0] !=
                    FLT_SCHEDULEDANIM_IGNORE)  ||
                (scheduled_animations[i].gpu_polygon_vals.xyz[0] > 0.01f)))
                ||
                ((scheduled_animations[i].final_values_not_adds &&
                scheduled_animations[i].gpu_polygon_vals.xyz[1] !=
                    FLT_SCHEDULEDANIM_IGNORE)  ||
            (scheduled_animations[i].gpu_polygon_vals.xyz[1] > 0.01f))))
        {
            scheduled_animations[i].deleted = true;
        }
    }
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
}

void delete_all_rgba_animations_targeting(
    const int32_t object_id)
{
    platform_mutex_lock(request_scheduled_anims_mutex_id);
    for (uint32_t i = 0; i < scheduled_animations_size; i++) {
        if (
            !scheduled_animations[i].deleted &&
            scheduled_animations[i].affected_object_id == (int32_t)object_id &&
            scheduled_animations[i].final_values_not_adds &&
            (scheduled_animations[i].gpu_polygon_material_vals.rgba[0] !=
                 FLT_SCHEDULEDANIM_IGNORE ||
             scheduled_animations[i].gpu_polygon_material_vals.rgba[1] !=
                 FLT_SCHEDULEDANIM_IGNORE ||
             scheduled_animations[i].gpu_polygon_material_vals.rgba[2] !=
                 FLT_SCHEDULEDANIM_IGNORE ||
             scheduled_animations[i].gpu_polygon_material_vals.rgba[3] !=
                 FLT_SCHEDULEDANIM_IGNORE))
        {
            scheduled_animations[i].deleted = true;
        }
    }
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
}

void delete_all_animations_targeting(const int32_t object_id) {
    platform_mutex_lock(request_scheduled_anims_mutex_id);
    for (uint32_t i = 0; i < scheduled_animations_size; i++) {
        if (
            !scheduled_animations[i].deleted &&
            scheduled_animations[i].committed &&
            scheduled_animations[i].affected_object_id == object_id)
        {
            scheduled_animations[i].deleted = true;
        }
    }
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
}

void delete_all_repeatforever_animations(void) {
    platform_mutex_lock(request_scheduled_anims_mutex_id);
    for (uint32_t i = 0; i < scheduled_animations_size; i++) {
        if (
            !scheduled_animations[i].deleted &&
            scheduled_animations[i].runs == 0 &&
            scheduled_animations[i].committed)
        {
            scheduled_animations[i].deleted = true;
        }
    }
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
}
