#include "scheduled_animations.h"

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
    
    to_construct->affected_sprite_id = -1;
    to_construct->affected_touchable_id = -1;
    
    common_memset_float(
         &to_construct->onfinish_gpu_polygon_material_muls,
         1.0f,
         sizeof(GPUPolygonMaterial));
    
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
            sizeof(GPUPolygon));
        common_memset_float(
            &return_value->gpu_polygon_material_vals,
            FLT_SCHEDULEDANIM_IGNORE,
            sizeof(GPUPolygonMaterial));
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
    
    if (to_commit->endpoints_not_deltas) {
        int32_t first_zp_i = 0;
        for (
            int32_t zp_i = 0;
            zp_i < (int32_t)zpolygons_to_render->size;
            zp_i++)
        {
            if (
                zpolygons_to_render->cpu_data[zp_i].sprite_id ==
                    to_commit->affected_sprite_id)
            {
                first_zp_i = zp_i;
                break;
            }
        }
        
        log_assert(first_zp_i >= 0);
        log_assert(first_zp_i < (int32_t)zpolygons_to_render->size);
        
        float * anim_gpu_vals = (float *)&to_commit->gpu_polygon_vals;
        float * anim_mat_vals = (float *)&to_commit->gpu_polygon_material_vals;
        
        float * orig_gpu_vals = (float *)&zpolygons_to_render->
            gpu_data[first_zp_i];
        float * orig_mat_vals = (float *)&zpolygons_to_render->
            gpu_materials[first_zp_i * MAX_MATERIALS_PER_POLYGON];
        
        for (
            uint32_t i = 0;
            i < (sizeof(GPUPolygon) / sizeof(float));
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
        
        for (
            uint32_t i = 0;
            i < (sizeof(GPUPolygonMaterial) / sizeof(float));
            i++)
        {
            if (anim_mat_vals[i] == FLT_SCHEDULEDANIM_IGNORE) {
                anim_mat_vals[i] = 0.0f; // delta 0 is the same as 'ignore'
            } else {
                // fetch the current value
                float delta_to_target = (anim_mat_vals[i] - orig_mat_vals[i]);
                anim_mat_vals[i] = delta_to_target;
            }
        }
        
        if (to_commit->delete_other_anims_targeting_same_object_id_on_commit) {
            for (
                uint32_t anim_i = 0;
                anim_i < scheduled_animations_size;
                anim_i++)
            {
                if (
                    scheduled_animations[anim_i].affected_sprite_id ==
                        to_commit->affected_sprite_id &&
                    scheduled_animations[anim_i].affected_touchable_id ==
                        to_commit->affected_touchable_id &&
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
    
    if (to_commit->affected_sprite_id < 0) {
        log_assert(to_commit->affected_touchable_id >= 0);
    } else {
        log_assert(to_commit->affected_touchable_id == -1);
    }
    
    if (to_commit->affected_touchable_id < 0) {
        log_assert(to_commit->affected_sprite_id >= 0);
    } else {
        log_assert(to_commit->affected_sprite_id == -1);
    }
    
    log_assert(to_commit->already_applied_t == 0.0f);
    
    if (to_commit->delete_other_anims_targeting_same_object_id_on_commit) {
        for (uint32_t i = 0; i < scheduled_animations_size; i++) {
            if (
                scheduled_animations[i].affected_sprite_id >= 0 &&
                scheduled_animations[i].affected_sprite_id ==
                    to_commit->affected_sprite_id &&
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

void scheduled_animations_request_evaporate_and_destroy(
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
            zpolygons_to_render->cpu_data[zp_i].sprite_id != object_id)
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

void scheduled_animations_request_shatter_and_destroy(
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
            zpolygons_to_render->cpu_data[zp_i].sprite_id != object_id)
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
        
        zpolygons_to_render->cpu_data[zp_i].deleted = true;
    }
}

void scheduled_animations_request_fade_and_destroy(
    const int32_t object_id,
    const uint64_t wait_before_first_run,
    const uint64_t duration_microseconds)
{
    log_assert(duration_microseconds > 0);
    
    // register scheduled animation
    ScheduledAnimation * fade_destroy = scheduled_animations_request_next(true);
    fade_destroy->endpoints_not_deltas = true;
    fade_destroy->affected_sprite_id = object_id;
    fade_destroy->start_timestamp =
        window_globals->this_frame_timestamp + wait_before_first_run;
    fade_destroy->end_timestamp =
        fade_destroy->start_timestamp + duration_microseconds;
    fade_destroy->lightsource_vals.reach = 0.0f;
    fade_destroy->gpu_polygon_material_vals.rgba[3] = 0.0f;
    fade_destroy->delete_object_when_finished = true;
    scheduled_animations_commit(fade_destroy);
}

void scheduled_animations_request_fade_to(
    const int32_t object_id,
    const uint64_t wait_before_first_run,
    const uint64_t duration_microseconds,
    const float target_alpha)
{
    log_assert(object_id >= 0);
    
    // register scheduled animation
    ScheduledAnimation * modify_alpha = scheduled_animations_request_next(true);
    modify_alpha->affected_sprite_id = object_id;
    modify_alpha->start_timestamp =
        window_globals->this_frame_timestamp + wait_before_first_run;
    modify_alpha->end_timestamp =
        modify_alpha->start_timestamp + duration_microseconds;
    modify_alpha->gpu_polygon_material_vals.rgba[3] = target_alpha;
    scheduled_animations_commit(modify_alpha);
}

float scheduled_animations_ease_bounce(const float t, const float bounces) {
    // Ensure t is clamped between 0.0f and 1.0f
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 0.0f;

    // Base oscillation using sine for smooth bouncing
    float oscillation = sinf(3.14159265359f * bounces * t); // 4 half-cycles for multiple bounces
    
    // Amplitude envelope to control bounce height and ensure 0 at endpoints
    float envelope = 4.0f * t * (1.0f - t); // Parabolic shape: peaks at t=0.5, zero at t=0 and t=1

    // Combine to get the bouncing effect
    float result = oscillation * envelope;

    // Scale to desired amplitude (adjust 0.5f for more/less extreme bounces)
    return result * 0.5f;
}

float scheduled_animations_ease_out_elastic(const float t) {
    const float c4 = (2.0f * (float)M_PI) / 3.0f;
    
    #if 0
    if (t == 0.0f || t == 1.0f) { return t; }
    #else
    if (fabsf(t) < 0.00001f) return 0.0f;
    if (fabsf(t - 1.0f) < 0.00001f) return 1.0f;
    #endif
    
    return
        powf(2, -10.0f * t) *
        sinf((t * 10.0f - 0.75f) * c4) + 1.0f;
}

float scheduled_animations_ease_out_quart(const float t) {
    return 1 - ((1 - t) * (1 - t) * (1 - t) * (1 - t));
}

float scheduled_animations_ease_lin_revert(const float t) {
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
            window_globals->this_frame_timestamp <
                anim->start_timestamp)
        {
            continue;
        }
        
        if (anim->already_applied_t >= 1.0f) {
            anim->deleted = true;
            anim->already_applied_t = 0.0f;
            if (animation_i == (int32_t)scheduled_animations_size - 1) {
                scheduled_animations_size -= 1;
            }
            
            if (anim->delete_object_when_finished) {
                delete_zlight(anim->affected_sprite_id);
                
                delete_zpolygon_object(anim->affected_sprite_id);
                
                delete_particle_effect(anim->affected_sprite_id);
            }
            
            continue;
        }
        
        log_assert(anim->end_timestamp > anim->start_timestamp);
        log_assert(
            window_globals->this_frame_timestamp >= anim->start_timestamp);
        
        uint64_t duration = anim->end_timestamp - anim->start_timestamp;
        uint64_t now =
            window_globals->this_frame_timestamp > anim->end_timestamp ?
                anim->end_timestamp :
                window_globals->this_frame_timestamp;
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
            case EASINGTYPE_EASEOUT_ELASTIC:
                t_eased =
                    scheduled_animations_ease_out_elastic(t_now);
                t_eased_already_applied =
                    scheduled_animations_ease_out_elastic(
                        anim->already_applied_t);
                break;
            case EASINGTYPE_SINGLE_BOUNCE:
                t_eased =
                    scheduled_animations_ease_bounce(t_now, 1.0f);
                t_eased_already_applied =
                    scheduled_animations_ease_bounce(
                        anim->already_applied_t, 1.0f);
                break;
            case EASINGTYPE_DOUBLE_BOUNCE:
                t_eased =
                    scheduled_animations_ease_bounce(t_now, 2.0f);
                t_eased_already_applied =
                    scheduled_animations_ease_bounce(
                        anim->already_applied_t, 2.0f);
                break;
            case EASINGTYPE_QUADRUPLE_BOUNCE:
                t_eased =
                    scheduled_animations_ease_bounce(t_now, 4.0f);
                t_eased_already_applied =
                    scheduled_animations_ease_bounce(
                        anim->already_applied_t, 4.0f);
                break;
            default:
                log_assert(0);
        }
        
        anim->already_applied_t = t_now;
        
        // Apply effects
        for (
            uint32_t zp_i = 0;
            zp_i < zpolygons_to_render->size;
            zp_i++)
        {
            if (
                (anim->affected_sprite_id >= 0 &&
                zpolygons_to_render->cpu_data[zp_i].sprite_id !=
                    anim->affected_sprite_id) ||
                (anim->affected_touchable_id >= 0 &&
                zpolygons_to_render->gpu_data[zp_i].touchable_id !=
                    anim->affected_touchable_id)
                ||
                zpolygons_to_render->cpu_data[zp_i].deleted)
            {
                continue;
            }
            
            float * anim_vals_ptr    =
                (float *)&anim->gpu_polygon_vals;
            float * target_vals_ptr =
                (float *)&zpolygons_to_render->gpu_data[zp_i];
            
            SIMD_FLOAT simd_t_now =
                simd_set1_float(t_eased);
            SIMD_FLOAT simd_t_b4  =
                simd_set1_float(t_eased_already_applied);
            
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
            #endif
            
            
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
            
            #if 0
            float flt_remaining_microseconds_this_run =
                (float)remaining_microseconds_at_start_of_run;
            
            SIMD_FLOAT simd_this_run_modifier =
                simd_div_floats(
                    simd_set1_float(flt_actual_elapsed_this_run),
                    simd_set1_float(flt_remaining_microseconds_this_run));
            #endif
            
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
    move_request->easing_type = EASINGTYPE_QUADRUPLE_BOUNCE;
    move_request->affected_sprite_id = (int32_t)object_id;
    move_request->gpu_polygon_vals.xyz[0] = magnitude * 0.10f;
    move_request->gpu_polygon_vals.xyz[1] = magnitude * 0.07f;
    move_request->gpu_polygon_vals.xyz[2] = magnitude * 0.01f;
    move_request->start_timestamp = window_globals->this_frame_timestamp;
    move_request->end_timestamp = move_request->start_timestamp + 300000;
    scheduled_animations_commit(move_request);
}

void scheduled_animations_request_bump(
    const int32_t object_id,
    const uint32_t wait)
{
    ScheduledAnimation * move_request =
        scheduled_animations_request_next(false);
    move_request->easing_type = EASINGTYPE_DOUBLE_BOUNCE;
    move_request->affected_sprite_id = (int32_t)object_id;
    move_request->gpu_polygon_vals.scale_factor = 0.25f;
    move_request->start_timestamp = window_globals->this_frame_timestamp;
    move_request->end_timestamp = move_request->start_timestamp + 200000;
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
            scheduled_animations[i].affected_sprite_id == (int32_t)object_id)
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
            scheduled_animations[i].affected_sprite_id == object_id)
        {
            scheduled_animations[i].deleted = true;
        }
    }
    platform_mutex_unlock(request_scheduled_anims_mutex_id);
}
