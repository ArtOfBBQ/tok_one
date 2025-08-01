#include "T1_scheduled_animations.h"

#if SCHEDULED_ANIMS_ACTIVE

#define FLT_SCHEDULEDANIM_IGNORE 0xFFFF

ScheduledAnimation * scheduled_animations;
uint32_t scheduled_animations_size = 0;

static uint32_t request_scheduled_anims_mutex_id = UINT32_MAX;

void scheduled_animations_init(void)
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
}

static void construct_scheduled_animationA(
    ScheduledAnimation * to_construct)
{
    common_memset_char(to_construct, 0, sizeof(ScheduledAnimation));
    log_assert(!to_construct->committed);
    
    to_construct->affected_zsprite_id = -1;
    to_construct->affected_touchable_id = -1;
    to_construct->runs = 1;
    
    log_assert(!to_construct->deleted);
    log_assert(!to_construct->committed);
}

ScheduledAnimation * scheduled_animations_request_next(
    bool32_t endpoints_not_deltas)
{
    platform_mutex_lock(request_scheduled_anims_mutex_id);
    log_assert(scheduled_animations_size < SCHEDULED_ANIMATIONS_ARRAYSIZE);
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
    construct_scheduled_animationA(return_value);
    
    log_assert(!return_value->committed);
    log_assert(!return_value->deleted);
    
    if (endpoints_not_deltas) {
        common_memset_float(
            &return_value->gpu_polygon_vals,
            FLT_SCHEDULEDANIM_IGNORE,
            sizeof(GPUzSprite));
        common_memset_float(
            &return_value->lightsource_vals,
            FLT_SCHEDULEDANIM_IGNORE,
            sizeof(zLightSource));
        return_value->endpoints_not_deltas = endpoints_not_deltas;
    }
    
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
    
    return return_value;
}

void scheduled_animations_commit(ScheduledAnimation * to_commit) {
    platform_mutex_lock(request_scheduled_anims_mutex_id);
    
    log_assert(to_commit->start_timestamp == 0);
    log_assert(to_commit->end_timestamp == 0);
    log_assert(to_commit->duration_us > 0);
    to_commit->start_timestamp = engine_globals->this_frame_timestamp_us;
    to_commit->end_timestamp =
        to_commit->start_timestamp + to_commit->duration_us;
    
    if (to_commit->endpoints_not_deltas) {
        int32_t first_zp_i = 0;
        for (
            int32_t zp_i = 0;
            zp_i < (int32_t)zsprites_to_render->size;
            zp_i++)
        {
            if (
                zsprites_to_render->cpu_data[zp_i].zsprite_id ==
                    to_commit->affected_zsprite_id)
            {
                first_zp_i = zp_i;
                break;
            }
        }
        
        log_assert(first_zp_i >= 0);
        log_assert(first_zp_i < (int32_t)zsprites_to_render->size);
        
        float * anim_gpu_vals = (float *)&to_commit->gpu_polygon_vals;
        
        float * orig_gpu_vals = (float *)&zsprites_to_render->
            gpu_data[first_zp_i];
        
        for (
            uint32_t i = 0;
            i < (sizeof(GPUzSprite) / sizeof(float));
            i++)
        {
            if (anim_gpu_vals[i] == FLT_SCHEDULEDANIM_IGNORE) {
                anim_gpu_vals[i] = 0.0f; // delta 0 is the same as 'ignore'
            } else {
                // fetch the current value
                float delta_to_target = anim_gpu_vals[i] - orig_gpu_vals[i];
                anim_gpu_vals[i] = delta_to_target;
            }
        }
        
        if (
            to_commit->delete_other_anims_targeting_same_object_id_on_commit)
        {
            for (
                uint32_t anim_i = 0;
                anim_i < scheduled_animations_size;
                anim_i++)
            {
                if (
                    scheduled_animations[anim_i].affected_zsprite_id ==
                        to_commit->affected_zsprite_id &&
                    scheduled_animations[anim_i].affected_touchable_id ==
                        to_commit->affected_touchable_id &&
                    scheduled_animations[anim_i].committed &&
                    scheduled_animations[anim_i].endpoints_not_deltas)
                {
                    scheduled_animations[anim_i].deleted = true;
                }
            }
        }
    }
    
    log_assert(!to_commit->deleted);
    log_assert(!to_commit->committed);
    log_assert(to_commit->end_timestamp > to_commit->start_timestamp);
    
    if (to_commit->affected_zsprite_id < 0) {
        log_assert(to_commit->affected_touchable_id >= 0);
    } else {
        log_assert(to_commit->affected_touchable_id == -1);
    }
    
    if (to_commit->affected_touchable_id < 0) {
        log_assert(to_commit->affected_zsprite_id >= 0);
    } else {
        log_assert(to_commit->affected_zsprite_id == -1);
    }
    
    log_assert(to_commit->already_applied_t == 0.0f);
    
    to_commit->committed = true;
    
    log_assert(to_commit->committed);
    log_assert(!to_commit->deleted);
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
}

void scheduled_animations_request_evaporate_and_destroy(
    const int32_t object_id,
    const uint64_t duration_us)
{
    #ifdef LOGGER_IGNORE_ASSERTS
    (void)duration_us;
    #endif
    
    log_assert(duration_us > 0);
    log_assert(object_id >= 0);
    
    for (
        uint32_t zp_i = 0;
        zp_i < zsprites_to_render->size;
        zp_i++)
    {
        if (
            zsprites_to_render->cpu_data[zp_i].deleted ||
            !zsprites_to_render->cpu_data[zp_i].committed ||
            zsprites_to_render->cpu_data[zp_i].zsprite_id != object_id)
        {
            continue;
        }
        
        #if PARTICLES_ACTIVE
        float duration_mod = (20000000.0f / (float)duration_us);
        
        ParticleEffect * vaporize_effect = next_particle_effect();
        vaporize_effect->zpolygon_cpu = zsprites_to_render->cpu_data[zp_i];
        vaporize_effect->zpolygon_gpu = zsprites_to_render->gpu_data[zp_i];
        
        uint64_t shattered_verts_size =
            (uint64_t)all_mesh_summaries[vaporize_effect->zpolygon_cpu.mesh_id].
                shattered_vertices_size;
        vaporize_effect->particle_spawns_per_second = (uint32_t)(
            (shattered_verts_size * 1000000) /
                (uint64_t)(duration_us + 1));
        vaporize_effect->pause_between_spawns = 10;
        vaporize_effect->vertices_per_particle = 3;
        vaporize_effect->particle_lifespan = duration_us;
        vaporize_effect->use_shattered_mesh = true;
        
        float xy_dist   =  0.0100f;
        float z_dist    = -0.0250f;
        float xyz_angle =  0.0100f;
        float rgb_delta =  0.0001f;
                
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
        #endif
        
        zsprites_to_render->cpu_data[zp_i].deleted = true;
    }
}

void scheduled_animations_request_shatter_and_destroy(
    const int32_t object_id,
    const uint64_t duration_us)
{
    #ifdef LOGGER_IGNORE_ASSERTS
    (void)duration_us;
    #endif
    
    log_assert(duration_us > 0);
    log_assert(duration_us < 1000000000);
    log_assert(object_id >= 0);
    
    for (
        uint32_t zp_i = 0;
        zp_i < zsprites_to_render->size;
        zp_i++)
    {
        if (zsprites_to_render->cpu_data[zp_i].deleted ||
            !zsprites_to_render->cpu_data[zp_i].committed ||
            zsprites_to_render->cpu_data[zp_i].zsprite_id != object_id)
        {
            continue;
        }
        
        #if PARTICLES_ACTIVE
        float duration_mod = (20000000.0f / (float)duration_us);
        
        ParticleEffect * shatter_effect = next_particle_effect();
        shatter_effect->zpolygon_cpu = zsprites_to_render->cpu_data[zp_i];
        shatter_effect->zpolygon_gpu = zsprites_to_render->gpu_data[zp_i];
        
        uint64_t shattered_verts_size =
            (uint64_t)all_mesh_summaries[shatter_effect->zpolygon_cpu.mesh_id].
                shattered_vertices_size;
        log_assert(shattered_verts_size > 0);
        shatter_effect->particle_spawns_per_second = (uint32_t)(
            (shattered_verts_size * 1000000) /
                (uint64_t)(duration_us + 1));
        shatter_effect->pause_between_spawns = 0;
        shatter_effect->vertices_per_particle = 6;
        shatter_effect->particle_lifespan = duration_us;
        shatter_effect->use_shattered_mesh = true;
        
        float xyz_dist = 0.02f;
        float xyz_angle = 0.05f;
        float rgb_delta = 0.05f;
        
        shatter_effect->gpustats_pertime_random_add_1.xyz[0] = -xyz_dist *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.xyz[1] = -xyz_dist *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.xyz[2] = -xyz_dist *
            duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.xyz_angle[0] =
            xyz_angle * duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.xyz_angle[1] =
            xyz_angle * duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.xyz_angle[2] =
            xyz_angle * duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.bonus_rgb[0] =
            rgb_delta * duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.bonus_rgb[1] =
            rgb_delta * duration_mod;
        shatter_effect->gpustats_pertime_random_add_1.bonus_rgb[2] =
            rgb_delta * duration_mod;
        shatter_effect->gpustats_pertime_random_add_2.xyz[0] =
            xyz_dist * duration_mod;
        shatter_effect->gpustats_pertime_random_add_2.xyz[1] =
            xyz_dist * duration_mod;
        shatter_effect->gpustats_pertime_random_add_2.xyz[2] =
            xyz_dist * duration_mod;
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
        #endif
        
        zsprites_to_render->cpu_data[zp_i].zsprite_id = -1;
        zsprites_to_render->cpu_data[zp_i].deleted = true;
    }
}

void scheduled_animations_request_fade_and_destroy(
    const int32_t  object_id,
    const uint64_t duration_us)
{
    log_assert(duration_us > 0);
    
    // register scheduled animation
    ScheduledAnimation * fade_destroy = scheduled_animations_request_next(true);
    fade_destroy->endpoints_not_deltas = true;
    fade_destroy->affected_zsprite_id = object_id;
    fade_destroy->duration_us = duration_us;
    fade_destroy->lightsource_vals.reach = 0.0f;
    fade_destroy->delete_object_when_finished = true;
    scheduled_animations_commit(fade_destroy);
}

void scheduled_animations_request_fade_to(
    const int32_t zsprite_id,
    const uint64_t duration_us,
    const float target_alpha)
{
    log_assert(zsprite_id >= 0);
    
    // register scheduled animation
    ScheduledAnimation * modify_alpha = scheduled_animations_request_next(true);
    modify_alpha->affected_zsprite_id = zsprite_id;
    modify_alpha->duration_us = duration_us;
    modify_alpha->gpu_polygon_vals.alpha = target_alpha;
    scheduled_animations_commit(modify_alpha);
}

float scheduled_animations_easing_bounce_zero_to_zero(
    const float t,
    const float bounces)
{
    // Ensure t is clamped between 0.0f and 1.0f
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 0.0f;
    
    // Base oscillation using sine for smooth bouncing
    float oscillation = sinf(3.14159265359f * bounces * t); // 4 half-cycles for multiple bounces
    
    // Amplitude envelope to control bounce height and ensure 0 at endpoints
    float envelope = bounces * t * (1.0f - t); // Parabolic shape: peaks at t=0.5, zero at t=0 and t=1
    
    // Combine to get the bouncing effect
    float result = oscillation * envelope;
    
    // Scale to desired amplitude (adjust 0.5f for more/less extreme bounces)
    return result * 0.5f;
}

float scheduled_animations_easing_pulse_zero_to_zero(
    const float t,
    const float pulses)
{
    // Ensure t is clamped between 0.0f and 1.0f
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 0.0f;
    
    // Base oscillation using absolute sine for non-negative pulsing
    float oscillation = fabsf(sinf(3.14159265359f * pulses * t)); // Non-negative, bounces half-cycles
    
    // Amplitude envelope to ensure 0 at endpoints
    float envelope = pulses * t * (1.0f - t); // Parabolic shape: peaks at t=0.5, zero at t=0 and t=1
    
    // Combine for pulsing effect
    float result = oscillation * envelope;
    
    // Scale to match original amplitude feel (adjust 0.5f for intensity)
    return result * 0.5f;
}

float scheduled_animations_easing_out_elastic_zero_to_one(const float t) {
    const float c4 = (2.0f * (float)M_PI) / 3.0f;
    
    if (t == 0.0f || t == 1.0f) { return t; }
    
    return
        powf(2, -10.0f * t) *
        sinf((t * 10.0f - 0.75f) * c4) + 1.0f;
}

float scheduled_animations_easing_out_quart(const float t) {
    return 1 - ((1 - t) * (1 - t) * (1 - t) * (1 - t));
}

float scheduled_animations_easing_lin_revert(const float t) {
    return 2.0f * t * (1.0f - t); // Peaks at 0.5f instead of 1.0f
}

void scheduled_animations_resolve(void)
{
    platform_mutex_lock(request_scheduled_anims_mutex_id);
    
    for (
        int32_t animation_i = (int32_t)scheduled_animations_size - 1;
        animation_i >= 0;
        animation_i--)
    {
        ScheduledAnimation * anim = &scheduled_animations[animation_i];
        
        if (
            anim->deleted ||
            !anim->committed ||
            engine_globals->this_frame_timestamp_us < anim->start_timestamp)
        {
            continue;
        }
        
        if (anim->already_applied_t >= 1.0f) {
            bool32_t delete = anim->runs == 1;
            bool32_t reduce_runs = anim->runs > 0;
            
            if (delete) {
                anim->deleted = true;
                if (animation_i == (int32_t)scheduled_animations_size - 1) {
                    scheduled_animations_size -= 1;
                }
                
                if (anim->delete_object_when_finished) {
                    delete_zlight(anim->affected_zsprite_id);
                    
                    zsprite_delete(anim->affected_zsprite_id);
                    
                    #if PARTICLES_ACTIVE
                    delete_particle_effect(anim->affected_zsprite_id);
                    #endif
                }
            } else {
                anim->start_timestamp = engine_globals->this_frame_timestamp_us;
                anim->end_timestamp =
                    anim->start_timestamp +
                    anim->duration_us;
            }
            
            if (reduce_runs) {
                anim->runs -= 1;
            }
            
            anim->already_applied_t = 0.0f;
            
            continue;
        }
        
        log_assert(anim->end_timestamp > anim->start_timestamp);
        log_assert(
            engine_globals->this_frame_timestamp_us >= anim->start_timestamp);
        
        uint64_t duration = anim->end_timestamp - anim->start_timestamp;
        uint64_t now =
            engine_globals->this_frame_timestamp_us > anim->end_timestamp ?
                anim->end_timestamp :
                engine_globals->this_frame_timestamp_us;
        log_assert(now >= anim->start_timestamp);
        log_assert(now <= anim->end_timestamp);
        log_assert(duration >= (anim->end_timestamp - now));
        
        uint64_t time_to_end = anim->end_timestamp - now;
        uint64_t elapsed_so_far = duration - time_to_end;
        
        float t_now = (float)elapsed_so_far / (float)duration;
        log_assert(t_now <= 1.0f);
        log_assert(t_now >= 0.0f);
        log_assert(t_now >= anim->already_applied_t);
        
        float t_eased = FLOAT32_MAX;
        float t_eased_already_applied = FLOAT32_MAX;
        
        switch (anim->easing_type) {
            case EASINGTYPE_NONE:
                t_eased = t_now;
                t_eased_already_applied = anim->already_applied_t;
                break;
            case EASINGTYPE_EASEOUT_ELASTIC_ZERO_TO_ONE:
                t_eased =
                    scheduled_animations_easing_out_elastic_zero_to_one(t_now);
                t_eased_already_applied =
                    scheduled_animations_easing_out_elastic_zero_to_one(
                        anim->already_applied_t);
                break;
            case EASINGTYPE_SINGLE_BOUNCE_ZERO_TO_ZERO:
                t_eased =
                    scheduled_animations_easing_bounce_zero_to_zero(t_now, 1.0f);
                t_eased_already_applied =
                    scheduled_animations_easing_bounce_zero_to_zero(
                        anim->already_applied_t, 1.0f);
                break;
            case EASINGTYPE_DOUBLE_BOUNCE_ZERO_TO_ZERO:
                t_eased =
                    scheduled_animations_easing_bounce_zero_to_zero(t_now, 2.0f);
                t_eased_already_applied =
                    scheduled_animations_easing_bounce_zero_to_zero(
                        anim->already_applied_t, 2.0f);
                break;
            case EASINGTYPE_QUADRUPLE_BOUNCE_ZERO_TO_ZERO:
                t_eased =
                    scheduled_animations_easing_bounce_zero_to_zero(t_now, 4.0f);
                t_eased_already_applied =
                    scheduled_animations_easing_bounce_zero_to_zero(
                        anim->already_applied_t, 4.0f);
                break;
            case EASINGTYPE_OCTUPLE_BOUNCE_ZERO_TO_ZERO:
                t_eased =
                    scheduled_animations_easing_bounce_zero_to_zero(
                        t_now, 8.0f);
                t_eased_already_applied =
                    scheduled_animations_easing_bounce_zero_to_zero(
                        anim->already_applied_t, 8.0f);
                break;
            case EASINGTYPE_SINGLE_PULSE_ZERO_TO_ZERO:
                t_eased =
                    scheduled_animations_easing_pulse_zero_to_zero(t_now, 1.0f);
                t_eased_already_applied =
                    scheduled_animations_easing_pulse_zero_to_zero(
                        anim->already_applied_t, 1.0f);
                break;
            case EASINGTYPE_OCTUPLE_PULSE_ZERO_TO_ZERO:
                t_eased =
                    scheduled_animations_easing_pulse_zero_to_zero(
                        t_now, 8.0f);
                t_eased_already_applied =
                    scheduled_animations_easing_pulse_zero_to_zero(
                        anim->already_applied_t, 8.0f);
                break;
            default:
                log_assert(0);
        }
        
        log_assert(anim->already_applied_t <= t_now);
        anim->already_applied_t = t_now;
        
                
        // Apply effects
        for (
            int32_t zp_i = 0;
            zp_i < (int32_t)zsprites_to_render->size;
            zp_i++)
        {
            if (
                (anim->affected_zsprite_id >= 0 &&
                zsprites_to_render->cpu_data[zp_i].zsprite_id !=
                    anim->affected_zsprite_id) ||
                (anim->affected_touchable_id >= 0 &&
                zsprites_to_render->gpu_data[zp_i].touchable_id !=
                    anim->affected_touchable_id)
                ||
                zsprites_to_render->cpu_data[zp_i].deleted)
            {
                continue;
            }
            
            float * anim_vals_ptr    =
                (float *)&anim->gpu_polygon_vals;
            float * target_vals_ptr =
                (float *)&zsprites_to_render->gpu_data[zp_i];
            
            SIMD_FLOAT simd_t_now =
                simd_set1_float(t_eased);
            SIMD_FLOAT simd_t_b4  =
                simd_set1_float(t_eased_already_applied);
            
            log_assert((sizeof(GPUzSprite) / 4) % SIMD_FLOAT_LANES == 0);
            for (
                uint32_t simd_step_i = 0;
                (simd_step_i * sizeof(float)) < sizeof(GPUzSprite);
                simd_step_i += SIMD_FLOAT_LANES)
            {
                SIMD_FLOAT simd_anim_vals =
                    simd_load_floats((anim_vals_ptr + simd_step_i));
                SIMD_FLOAT simd_target_vals =
                    simd_load_floats((target_vals_ptr + simd_step_i));
                
                SIMD_FLOAT simd_t_now_deltas =
                    simd_mul_floats(
                        simd_anim_vals,
                        simd_t_now);
                SIMD_FLOAT simd_t_previous_deltas =
                    simd_mul_floats(
                        simd_anim_vals,
                        simd_t_b4);
                
                simd_t_now_deltas = simd_sub_floats(
                    simd_t_now_deltas,
                    simd_t_previous_deltas);
                
                simd_store_floats(
                    (target_vals_ptr + simd_step_i),
                    simd_add_floats(
                        simd_target_vals,
                        simd_t_now_deltas));
            }
        }
        
        #if 0
        log_assert((sizeof(zLightSource) / 4) % SIMD_FLOAT_LANES == 0);
        for (
            uint32_t light_i = 0;
            light_i < zlights_to_apply_size;
            light_i++)
        {
            if (
                zlights_to_apply[light_i].object_id !=
                    anim->affected_sprite_id ||
                zlights_to_apply[light_i].deleted ||
                !zlights_to_apply[light_i].committed)
            {
                continue;
            }
            
            float flt_actual_elapsed_this_run =
                (float)actual_elapsed_this_run;
            
            float * anim_vals_ptr = (float *)&anim->lightsource_vals;
            float * target_vals_ptr = (float *)&zlights_to_apply[light_i];
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
        #endif
    }
    
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
}

void scheduled_animations_request_dud_dance(
    const int32_t object_id,
    const float magnitude)
{
    ScheduledAnimation * move_request =
        scheduled_animations_request_next(false);
    move_request->easing_type = EASINGTYPE_QUADRUPLE_BOUNCE_ZERO_TO_ZERO;
    move_request->affected_zsprite_id = (int32_t)object_id;
    move_request->gpu_polygon_vals.xyz[0] = magnitude * 0.05f;
    move_request->gpu_polygon_vals.xyz[1] = magnitude * 0.035f;
    move_request->gpu_polygon_vals.xyz[2] = magnitude * 0.005f;
    move_request->duration_us = 300000;
    scheduled_animations_commit(move_request);
}

void scheduled_animations_request_bump(
    const int32_t object_id,
    const uint32_t wait)
{
    #ifndef LOGGER_IGNORE_ASSERTS
    log_assert(wait == 0.0f);
    #else
    (void)wait;
    #endif
    
    ScheduledAnimation * move_request =
        scheduled_animations_request_next(false);
    move_request->easing_type = EASINGTYPE_DOUBLE_BOUNCE_ZERO_TO_ZERO;
    move_request->affected_zsprite_id = (int32_t)object_id;
    move_request->gpu_polygon_vals.scale_factor = 0.25f;
    move_request->duration_us = 200000;
    scheduled_animations_commit(move_request);
}

void scheduled_animations_delete_all(void)
{
    platform_mutex_lock(request_scheduled_anims_mutex_id);
    for (uint32_t i = 0; i < scheduled_animations_size; i++) {
        scheduled_animations[i].deleted = true;
    }
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
}

void scheduled_animations_delete_endpoint_anims_targeting(
    const int32_t object_id)
{
    platform_mutex_lock(request_scheduled_anims_mutex_id);
    for (uint32_t i = 0; i < scheduled_animations_size; i++) {
        if (
            !scheduled_animations[i].deleted &&
            scheduled_animations[i].endpoints_not_deltas &&
            scheduled_animations[i].affected_zsprite_id == (int32_t)object_id)
        {
            scheduled_animations[i].deleted = true;
        }
    }
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
}

void scheduled_animations_delete_all_anims_targeting(const int32_t object_id) {
    platform_mutex_lock(request_scheduled_anims_mutex_id);
    for (uint32_t i = 0; i < scheduled_animations_size; i++) {
        if (
            !scheduled_animations[i].deleted &&
            scheduled_animations[i].committed &&
            scheduled_animations[i].affected_zsprite_id == object_id)
        {
            scheduled_animations[i].deleted = true;
        }
    }
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
}

#endif // SCHEDULED_ANIMS_ACTIVE
