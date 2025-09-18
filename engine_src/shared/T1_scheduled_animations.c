#include "T1_scheduled_animations.h"

#if T1_SCHEDULED_ANIMS_ACTIVE == T1_ACTIVE

#define FLT_SCHEDULEDANIM_IGNORE 0xFFFF

T1ScheduledAnimation * scheduled_animations;
uint32_t scheduled_animations_size = 0;

typedef struct ScheduledAnimationState {
    T1GPUzSprite zsprite_final_pos;
    T1GPUzSprite zsprite_deltas[2];
} ScheduledAnimationState;

static ScheduledAnimationState * sas = NULL;

static uint32_t request_scheduled_anims_mutex_id = UINT32_MAX;

void T1_scheduled_animations_init(void)
{
    scheduled_animations = (T1ScheduledAnimation *)T1_mem_malloc_from_unmanaged(
        sizeof(T1ScheduledAnimation) * SCHEDULED_ANIMATIONS_ARRAYSIZE);
    T1_std_memset(
        scheduled_animations,
        0,
        sizeof(T1ScheduledAnimation));
    scheduled_animations[0].deleted = true;
    
    sas = (ScheduledAnimationState *)T1_mem_malloc_from_unmanaged(
        sizeof(ScheduledAnimationState));
    T1_std_memset(
        sas,
        0,
        sizeof(ScheduledAnimationState));
    
    for (uint32_t i = 1; i < SCHEDULED_ANIMATIONS_ARRAYSIZE; i++) {
        T1_std_memcpy(
            &scheduled_animations[i],
            &scheduled_animations[0],
            sizeof(T1ScheduledAnimation));
        log_assert(scheduled_animations[i].deleted);
    }
    
    request_scheduled_anims_mutex_id = platform_init_mutex_and_return_id();
}

static void construct_scheduled_animationA(
    T1ScheduledAnimation * to_construct)
{
    T1_std_memset(to_construct, 0, sizeof(T1ScheduledAnimation));
    log_assert(!to_construct->committed);
    
    to_construct->affected_zsprite_id = -1;
    to_construct->affected_touchable_id = -1;
    to_construct->runs = 1;
    
    log_assert(!to_construct->deleted);
    log_assert(!to_construct->committed);
}

T1ScheduledAnimation * T1_scheduled_animations_request_next(
    bool32_t endpoints_not_deltas)
{
    platform_mutex_lock(request_scheduled_anims_mutex_id);
    log_assert(scheduled_animations_size < SCHEDULED_ANIMATIONS_ARRAYSIZE);
    T1ScheduledAnimation * return_value = NULL;
    
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
        T1_std_memset_f32(
            &return_value->gpu_polygon_vals,
            FLT_SCHEDULEDANIM_IGNORE,
            sizeof(T1GPUzSprite));
        T1_std_memset_f32(
            &return_value->lightsource_vals,
            FLT_SCHEDULEDANIM_IGNORE,
            sizeof(zLightSource));
        return_value->endpoints_not_deltas = endpoints_not_deltas;
    }
    
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
    
    return return_value;
}

static float T1_scheduled_animations_easing_bounce_zero_to_zero(
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

static float T1_scheduled_animations_easing_pulse_zero_to_zero(
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

static float T1_scheduled_animations_easing_out_elastic_zero_to_one(const float t) {
    const float c4 = (2.0f * (float)M_PI) / 3.0f;
    
    if (t == 0.0f || t == 1.0f) { return t; }
    
    return
        powf(2, -10.0f * t) *
        sinf((t * 10.0f - 0.75f) * c4) + 1.0f;
}

#if 0
static float T1_scheduled_animations_easing_out_quart(const float t) {
    return 1 - ((1 - t) * (1 - t) * (1 - t) * (1 - t));
}

static float T1_scheduled_animations_easing_lin_revert(const float t) {
    return 2.0f * t * (1.0f - t); // Peaks at 0.5f instead of 1.0f
}
#endif

static T1TPair t_to_eased_t(
    const T1TPair t,
    const EasingType easing_type)
{
    T1TPair return_val;
    
    switch (easing_type) {
        case EASINGTYPE_NONE:
            return_val = t;
            break;
        case EASINGTYPE_EASEOUT_ELASTIC_ZERO_TO_ONE:
            return_val.now =
                T1_scheduled_animations_easing_out_elastic_zero_to_one(
                    t.now);
            return_val.applied =
                T1_scheduled_animations_easing_out_elastic_zero_to_one(
                    t.applied);
            break;
        case EASINGTYPE_SINGLE_BOUNCE_ZERO_TO_ZERO:
            return_val.now =
                T1_scheduled_animations_easing_bounce_zero_to_zero(
                    t.now, 1.0f);
            return_val.applied =
                T1_scheduled_animations_easing_bounce_zero_to_zero(
                    t.applied, 1.0f);
            break;
        case EASINGTYPE_DOUBLE_BOUNCE_ZERO_TO_ZERO:
            return_val.now =
                T1_scheduled_animations_easing_bounce_zero_to_zero(
                    t.now, 2.0f);
            return_val.applied =
                T1_scheduled_animations_easing_bounce_zero_to_zero(
                    t.applied, 2.0f);
            break;
        case EASINGTYPE_QUADRUPLE_BOUNCE_ZERO_TO_ZERO:
            return_val.now =
                T1_scheduled_animations_easing_bounce_zero_to_zero(
                    t.now, 4.0f);
            return_val.applied =
                T1_scheduled_animations_easing_bounce_zero_to_zero(
                    t.applied, 4.0f);
            break;
        case EASINGTYPE_OCTUPLE_BOUNCE_ZERO_TO_ZERO:
            return_val.now =
                T1_scheduled_animations_easing_bounce_zero_to_zero(
                    t.now, 8.0f);
            return_val.applied =
                T1_scheduled_animations_easing_bounce_zero_to_zero(
                    t.applied, 8.0f);
            break;
        case EASINGTYPE_SINGLE_PULSE_ZERO_TO_ZERO:
            return_val.now =
                T1_scheduled_animations_easing_pulse_zero_to_zero(
                    t.now, 1.0f);
            return_val.applied =
                T1_scheduled_animations_easing_pulse_zero_to_zero(
                    t.applied, 1.0f);
            break;
        case EASINGTYPE_OCTUPLE_PULSE_ZERO_TO_ZERO:
            return_val.now =
                T1_scheduled_animations_easing_pulse_zero_to_zero(
                    t.now, 8.0f);
            return_val.applied =
                T1_scheduled_animations_easing_pulse_zero_to_zero(
                    t.applied, 8.0f);
            break;
        default:
            log_assert(0);
    }
    
    return return_val;
}

static void apply_animation_effects_for_given_eased_t(
    T1TPair t,
    T1ScheduledAnimation * anim,
    T1GPUzSprite * recip)
{
    float * anim_vals_ptr    = (float *)&anim->gpu_polygon_vals;
    float * target_vals_ptr = (float *)recip;
    
    SIMD_FLOAT simd_t_now =
        simd_set1_float(t.now);
    SIMD_FLOAT simd_t_b4  =
        simd_set1_float(t.applied);
    
    log_assert((sizeof(T1GPUzSprite) / 4) % SIMD_FLOAT_LANES == 0);
    for (
        uint32_t simd_step_i = 0;
        (simd_step_i * sizeof(float)) < sizeof(T1GPUzSprite);
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
    
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    log_assert(recip->ignore_camera >= -0.05f);
    log_assert(recip->ignore_camera <= 1.05f);
    log_assert(recip->ignore_lighting >= -0.05f);
    log_assert(recip->ignore_lighting <= 1.05f);
    log_assert(recip->remove_shadow >= 0);
    log_assert(recip->remove_shadow <= 1);
    // log_assert(recip->alpha >= -0.1f);
    // log_assert(recip->alpha <=  1.1f);
    log_assert(recip->scale_factor >    0.0f);
    log_assert(recip->scale_factor < 1000.0f);
    // log_assert(recip->xyz_multiplier[0] > 0.0f);
    // log_assert(recip->xyz_multiplier[1] > 0.0f);
    // log_assert(recip->xyz_multiplier[2] > 0.0f);
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
}

static void T1_scheduled_animations_get_projected_final_position_for(
    const int32_t zp_i,
    T1GPUzSprite * recip)
{
    log_assert(zp_i >= 0);
    log_assert((uint32_t)zp_i < zsprites_to_render->size);
    
    const int32_t zsprite_id = zsprites_to_render->cpu_data[zp_i].zsprite_id;
    *recip = zsprites_to_render->gpu_data[zp_i];
    
    for (uint32_t sa_i = 0; sa_i < scheduled_animations_size; sa_i++) {
        if (
            scheduled_animations[sa_i].affected_zsprite_id == zsprite_id &&
            !scheduled_animations[sa_i].deleted &&
            scheduled_animations[sa_i].committed &&
            scheduled_animations[sa_i].duration_us > 0)
        {
            T1TPair t;
            t.now = 1.0f;
            t.applied = scheduled_animations[sa_i].already_applied_t;
            
            t = t_to_eased_t(t, scheduled_animations[sa_i].easing_type);
            
            apply_animation_effects_for_given_eased_t(
                t,
                &scheduled_animations[sa_i],
                recip);
        }
    }
}

void T1_scheduled_animations_commit(T1ScheduledAnimation * to_commit) {
    platform_mutex_lock(request_scheduled_anims_mutex_id);
    
    log_assert(to_commit->duration_us > 0);
    
    if (to_commit->delete_other_anims_targeting_same_object_id_on_commit)
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
    
    if (to_commit->endpoints_not_deltas) {
        int32_t first_zp_i = -1;
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
        
        if (first_zp_i < 0) {
            to_commit->deleted = true;
            platform_mutex_unlock(request_scheduled_anims_mutex_id);
            return;
        }
        log_assert(first_zp_i < (int32_t)zsprites_to_render->size);
        
        float * anim_gpu_vals = (float *)&to_commit->gpu_polygon_vals;
        
        T1_scheduled_animations_get_projected_final_position_for(
            first_zp_i,
            &sas->zsprite_final_pos);
        
        float * orig_gpu_vals = (float *)&sas->zsprite_final_pos;
        
        for (
            uint32_t i = 0;
            i < (sizeof(T1GPUzSprite) / sizeof(float));
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
    }
    
    log_assert(!to_commit->deleted);
    log_assert(!to_commit->committed);
    
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
    
    to_commit->remaining_pause_us = to_commit->pause_us;
    to_commit->remaining_duration_us = to_commit->duration_us;
    
    log_assert(to_commit->remaining_duration_us > 0);
    to_commit->committed = true;
    
    log_assert(to_commit->committed);
    log_assert(!to_commit->deleted);
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
}

void T1_scheduled_animations_request_evaporate_and_destroy(
    const int32_t object_id,
    const uint64_t duration_us)
{
    #if T1_LOGGER_ASSERTS_ACTIVE
    #else
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
        
        #if T1_PARTICLES_ACTIVE == T1_ACTIVE
        float duration_mod = (20000000.0f / (float)duration_us);
        
        T1ParticleEffect * vaporize_effect = T1_particle_get_next();
        vaporize_effect->zpolygon_cpu = zsprites_to_render->cpu_data[zp_i];
        vaporize_effect->zpolygon_gpu = zsprites_to_render->gpu_data[zp_i];
        
        uint64_t shattered_verts_size =
            (uint64_t)all_mesh_summaries[vaporize_effect->zpolygon_cpu.mesh_id].
                shattered_vertices_size;
        vaporize_effect->spawns_per_sec = (uint32_t)(
            (shattered_verts_size * 1000000) /
                (uint64_t)(duration_us + 1));
        vaporize_effect->pause_per_spawn = 10;
        vaporize_effect->verts_per_particle = 3;
        vaporize_effect->lifespan = duration_us;
        vaporize_effect->shattered = true;
        
        float xy_dist   =  0.0065f;
        float z_dist    = -0.0130f;
        float xyz_angle =  0.0100f;
        float rgb_delta =  0.00005f;
        
        vaporize_effect->pertime_rand_add[0].xyz[0] = -xy_dist *
            duration_mod;
        vaporize_effect->pertime_rand_add[0].xyz[1] = -xy_dist *
            duration_mod;
        vaporize_effect->pertime_rand_add[0].xyz[2] = z_dist *
            duration_mod;
        vaporize_effect->pertime_rand_add[0].xyz_angle[0] =
            xyz_angle * duration_mod;
        vaporize_effect->pertime_rand_add[0].xyz_angle[1] =
            xyz_angle * duration_mod;
        vaporize_effect->pertime_rand_add[0].xyz_angle[2] =
            xyz_angle * duration_mod;
        vaporize_effect->pertime_rand_add[0].bonus_rgb[0] =
            rgb_delta * duration_mod;
        vaporize_effect->pertime_rand_add[1].xyz[0] =
            xy_dist * duration_mod;
        vaporize_effect->pertime_rand_add[1].xyz[1] =
            xy_dist * duration_mod;
        vaporize_effect->pertime_rand_add[1].xyz[2] =
            z_dist * duration_mod;
        vaporize_effect->pertime_rand_add[1].xyz_angle[0] =
            -xyz_angle * duration_mod;
        vaporize_effect->pertime_rand_add[1].xyz_angle[1] =
            -xyz_angle * duration_mod;
        vaporize_effect->pertime_rand_add[1].xyz_angle[2] =
            -xyz_angle * duration_mod;
        vaporize_effect->perexptime_add.scale_factor = -1.5f;
        
        vaporize_effect->loops = 1;
        vaporize_effect->cast_light = false;
        
        T1_particle_commit(vaporize_effect);
        #elif T1_PARTICLES_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        zsprites_to_render->cpu_data[zp_i].deleted = true;
    }
}

void T1_scheduled_animations_request_shatter_and_destroy(
    const int32_t object_id,
    const uint64_t duration_us)
{
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    (void)duration_us;
    #else
    #error
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
        
        #if T1_PARTICLES_ACTIVE == T1_ACTIVE
        float duration_mod = (20000000.0f / (float)duration_us);
        
        T1ParticleEffect * shatter_effect = T1_particle_get_next();
        shatter_effect->zpolygon_cpu = zsprites_to_render->cpu_data[zp_i];
        shatter_effect->zpolygon_gpu = zsprites_to_render->gpu_data[zp_i];
        
        uint64_t shattered_verts_size =
            (uint64_t)all_mesh_summaries[shatter_effect->zpolygon_cpu.mesh_id].
                shattered_vertices_size;
        log_assert(shattered_verts_size > 0);
        shatter_effect->spawns_per_sec = (uint32_t)(
            (shattered_verts_size * 1000000) /
                (uint64_t)(duration_us + 1));
        shatter_effect->pause_per_spawn = 0;
        shatter_effect->verts_per_particle = 6;
        shatter_effect->lifespan = duration_us;
        shatter_effect->shattered = true;
        
        float xyz_dist = 0.02f;
        float xyz_angle = 0.05f;
        float rgb_delta = 0.05f;
        
        shatter_effect->pertime_rand_add[0].xyz[0] = -xyz_dist *
            duration_mod;
        shatter_effect->pertime_rand_add[0].xyz[1] = -xyz_dist *
            duration_mod;
        shatter_effect->pertime_rand_add[0].xyz[2] = -xyz_dist *
            duration_mod;
        shatter_effect->pertime_rand_add[0].xyz_angle[0] =
            xyz_angle * duration_mod;
        shatter_effect->pertime_rand_add[0].xyz_angle[1] =
            xyz_angle * duration_mod;
        shatter_effect->pertime_rand_add[0].xyz_angle[2] =
            xyz_angle * duration_mod;
        shatter_effect->pertime_rand_add[0].bonus_rgb[0] =
            rgb_delta * duration_mod;
        shatter_effect->pertime_rand_add[0].bonus_rgb[1] =
            rgb_delta * duration_mod;
        shatter_effect->pertime_rand_add[0].bonus_rgb[2] =
            rgb_delta * duration_mod;
        shatter_effect->pertime_rand_add[1].xyz[0] =
            xyz_dist * duration_mod;
        shatter_effect->pertime_rand_add[1].xyz[1] =
            xyz_dist * duration_mod;
        shatter_effect->pertime_rand_add[1].xyz[2] =
            xyz_dist * duration_mod;
        shatter_effect->pertime_rand_add[1].xyz_angle[0] =
            -xyz_angle * duration_mod;
        shatter_effect->pertime_rand_add[1].xyz_angle[1] =
            -xyz_angle * duration_mod;
        shatter_effect->pertime_rand_add[1].xyz_angle[2] =
            -xyz_angle * duration_mod;
        shatter_effect->perexptime_add.scale_factor = -0.07f *
            duration_mod;
        
        shatter_effect->loops = 1;
        shatter_effect->cast_light = false;
        
        log_assert(!shatter_effect->zpolygon_cpu.alpha_blending_enabled);
        
        T1_particle_commit(shatter_effect);
        #elif T1_PARTICLES_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        zsprites_to_render->cpu_data[zp_i].zsprite_id = -1;
        zsprites_to_render->cpu_data[zp_i].deleted = true;
    }
}

void T1_scheduled_animations_request_fade_and_destroy(
    const int32_t  object_id,
    const uint64_t duration_us)
{
    log_assert(duration_us > 0);
    
    // register scheduled animation
    T1ScheduledAnimation * fade_destroy = T1_scheduled_animations_request_next(true);
    fade_destroy->endpoints_not_deltas = true;
    fade_destroy->affected_zsprite_id = object_id;
    fade_destroy->duration_us = duration_us;
    fade_destroy->lightsource_vals.reach = 0.0f;
    fade_destroy->gpu_polygon_vals.alpha = 0.0f;
    fade_destroy->delete_object_when_finished = true;
    T1_scheduled_animations_commit(fade_destroy);
}

void T1_scheduled_animations_request_fade_to(
    const int32_t zsprite_id,
    const uint64_t duration_us,
    const float target_alpha)
{
    log_assert(zsprite_id >= 0);
    
    // register scheduled animation
    T1ScheduledAnimation * modify_alpha = T1_scheduled_animations_request_next(true);
    modify_alpha->affected_zsprite_id = zsprite_id;
    modify_alpha->duration_us = duration_us;
    modify_alpha->gpu_polygon_vals.alpha = target_alpha;
    T1_scheduled_animations_commit(modify_alpha);
}

void T1_scheduled_animations_resolve(void)
{
    platform_mutex_lock(request_scheduled_anims_mutex_id);
    
    for (
        int32_t animation_i = (int32_t)scheduled_animations_size - 1;
        animation_i >= 0;
        animation_i--)
    {
        T1ScheduledAnimation * anim = &scheduled_animations[animation_i];
        
        if (
            anim->deleted ||
            !anim->committed)
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
                    T1_particle_delete(anim->affected_zsprite_id);
                    #endif
                }
            } else {
                anim->remaining_duration_us = anim->duration_us;
                anim->remaining_pause_us = anim->pause_us;
            }
            
            if (reduce_runs) {
                anim->runs -= 1;
            }
            
            anim->already_applied_t = 0.0f;
            
            continue;
        }
        
        uint64_t elapsed = engine_globals->elapsed;
        
        if (anim->remaining_pause_us > 0) {
            
            if (elapsed <= anim->remaining_pause_us) {
                anim->remaining_pause_us -= elapsed;
                continue;
            } else {
                elapsed -= anim->remaining_pause_us;
                anim->remaining_pause_us = 0;
            }
        }
        
        log_assert(anim->remaining_duration_us > 0);
        
        if (elapsed < anim->remaining_duration_us)
        {
            anim->remaining_duration_us -= elapsed;
        } else {
            anim->remaining_duration_us = 0;
        }
        
        uint64_t elapsed_so_far =
            anim->duration_us - anim->remaining_duration_us;
        log_assert(elapsed_so_far <= anim->duration_us);
        
        T1TPair t;
        t.now = (float)elapsed_so_far / (float)anim->duration_us;
        log_assert(t.now <= 1.0f);
        log_assert(t.now >= 0.0f);
        log_assert(t.now >= anim->already_applied_t);
        t.applied = anim->already_applied_t;
        
        log_assert(anim->already_applied_t <= t.now);
        anim->already_applied_t = t.now;
        
        t = t_to_eased_t(t, anim->easing_type);
        
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
            
            apply_animation_effects_for_given_eased_t(
                t,
                anim,
                zsprites_to_render->gpu_data + zp_i);
        }
    }
    
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
}

void T1_scheduled_animations_request_dud_dance(
    const int32_t object_id,
    const float magnitude)
{
    T1ScheduledAnimation * move_request =
        T1_scheduled_animations_request_next(false);
    move_request->easing_type = EASINGTYPE_QUADRUPLE_BOUNCE_ZERO_TO_ZERO;
    move_request->affected_zsprite_id = (int32_t)object_id;
    move_request->gpu_polygon_vals.xyz[0] = magnitude * 0.05f;
    move_request->gpu_polygon_vals.xyz[1] = magnitude * 0.035f;
    move_request->gpu_polygon_vals.xyz[2] = magnitude * 0.005f;
    move_request->duration_us = 300000;
    T1_scheduled_animations_commit(move_request);
}

void T1_scheduled_animations_request_bump(
    const int32_t object_id,
    const uint32_t wait)
{
    #if T1_LOGGER_ASSERTS_ACTIVE
    log_assert(wait == 0.0f);
    #else
    (void)wait;
    #endif
    
    T1ScheduledAnimation * move_request =
        T1_scheduled_animations_request_next(false);
    move_request->easing_type = EASINGTYPE_DOUBLE_BOUNCE_ZERO_TO_ZERO;
    move_request->affected_zsprite_id = (int32_t)object_id;
    move_request->gpu_polygon_vals.scale_factor = 0.25f;
    move_request->duration_us = 200000;
    T1_scheduled_animations_commit(move_request);
}

void T1_scheduled_animations_delete_all(void)
{
    platform_mutex_lock(request_scheduled_anims_mutex_id);
    for (uint32_t i = 0; i < scheduled_animations_size; i++) {
        scheduled_animations[i].deleted = true;
    }
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
}

void T1_scheduled_animations_delete_endpoint_anims_targeting(
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

void T1_scheduled_animations_delete_all_anims_targeting(
    const int32_t object_id)
{
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

void T1_scheduled_animations_set_ignore_camera_but_retain_screenspace_pos(
    const int32_t zsprite_id,
    const float new_ignore_camera)
{
    T1GPUzSprite * zs = NULL;
    
    for (uint32_t i = 0; i < zsprites_to_render->size; i++)
    {
        if (zsprites_to_render->cpu_data[i].zsprite_id == zsprite_id) {
            zs = zsprites_to_render->gpu_data + i;
        }
    }
    
    if (zs->ignore_camera == new_ignore_camera) {
        return;
    }
    
    // For now we're only supporting the easy case of a full toggle
    bool32_t is_near_zero =
        zs->ignore_camera > -0.01f &&
        zs->ignore_camera <  0.01f;
    #if T1_LOGGER_ASSERTS_ACTIVE
    bool32_t is_near_one =
        zs->ignore_camera >  0.99f &&
        zs->ignore_camera <  1.01f;
    #endif
    log_assert(is_near_zero || is_near_one);
    
    if (is_near_zero) {
        log_assert(new_ignore_camera == 1.0f);
        
        zs->xyz[0] -= camera.xyz[0];
        zs->xyz[1] -= camera.xyz[1];
        zs->xyz[2] -= camera.xyz[2];
        x_rotate_f3(zs->xyz, -camera.xyz_angle[0]);
        y_rotate_f3(zs->xyz, -camera.xyz_angle[1]);
        z_rotate_f3(zs->xyz, -camera.xyz_angle[2]);
        
        #if 1
        // This is a hack, an approximation
        zs->xyz_angle[0] -= camera.xyz_angle[0];
        zs->xyz_angle[1] -= camera.xyz_angle[1];
        zs->xyz_angle[2] -= camera.xyz_angle[2];
        #endif
        
        zs->ignore_camera = 1.0f;
    } else {
        log_assert(is_near_one);
        
        z_rotate_f3(zs->xyz, camera.xyz_angle[2]);
        y_rotate_f3(zs->xyz, camera.xyz_angle[1]);
        x_rotate_f3(zs->xyz, camera.xyz_angle[0]);
        
        zs->xyz[0] += camera.xyz[0];
        zs->xyz[1] += camera.xyz[1];
        zs->xyz[2] += camera.xyz[2];
        
        #if 1
        // This is a hack, an approximation
        zs->xyz_angle[0] += camera.xyz_angle[0];
        zs->xyz_angle[1] += camera.xyz_angle[1];
        zs->xyz_angle[2] += camera.xyz_angle[2];
        #endif
        
        zs->ignore_camera = 0.0f;
    }
}

#elif T1_SCHEDULED_ANIMS_ACTIVE == T1_INACTIVE
#else
#error
#endif // T1_SCHEDULED_ANIMS_ACTIVE
