#include "T1_particle.h"

#if T1_PARTICLES_ACTIVE == T1_ACTIVE

#define add_variance(x, variance, randnum, randnum2) if (variance > 0) { x += ((float)(randnum % variance) * 0.01f); x -= ((float)(randnum2 % variance) * 0.01f); }

T1ParticleEffect * T1_particle_effects = NULL;
uint32_t T1_particle_effects_size = 0;

void T1_particle_effect_construct(
    T1ParticleEffect * to_construct)
{
    T1_std_memset(to_construct, 0, sizeof(T1ParticleEffect));
    
    to_construct->zsprite_id = -1;
    
    to_construct->random_seed = (uint32_t)
        T1_rand() % (RANDOM_SEQUENCE_SIZE - 100);
    to_construct->spawns_per_loop = 30;
    to_construct->spawn_lifespan = 2000000;
    to_construct->modifiers_size = 1;
}

T1ParticleEffect * T1_particle_get_next(void)
{
    T1ParticleEffect * return_value = NULL;
    
    for (uint32_t i = 0; i < T1_particle_effects_size; i++) {
        if (T1_particle_effects[i].deleted) {
            return_value = &T1_particle_effects[i];
            break;
        }
    }
    
    if (return_value == NULL) {
        log_assert(T1_particle_effects_size + 1 <
            PARTICLE_EFFECTS_SIZE);
        return_value = &T1_particle_effects[T1_particle_effects_size];
        T1_particle_effects_size += 1;
    }
    
    T1_particle_effect_construct(return_value);
    return return_value;
}

void T1_particle_commit(T1ParticleEffect * to_request)
{
    if (!T1_global->clientlogic_early_startup_finished) {
        log_dump_and_crash(
            "You can't commit particle effects during early startup.");
        return;
    }
    
    log_assert(!to_request->deleted);
    
    // Reminder: The particle effect is not committed, but the zpoly should be
    log_assert(!to_request->committed);
    
    log_assert(to_request->spawn_lifespan > 0);
    log_assert(to_request->elapsed == 0);
    log_assert(to_request->spawns_per_loop > 0);
    
    to_request->committed = true;
}

void T1_particle_delete(int32_t with_object_id)
{
    for (uint32_t i = 0; i < T1_particle_effects_size; i++) {
        if (T1_particle_effects[i].zsprite_id == with_object_id) {
            T1_particle_effects[i].deleted = true;
        }
    }
    
    while (
        T1_particle_effects_size > 0 &&
        T1_particle_effects[T1_particle_effects_size - 1].deleted)
    {
        T1_particle_effects_size -= 1;
    }
}

static float T1_particle_get_height(
    T1ParticleEffect * pe)
{
    float out = 0.0f;
    
    for (uint32_t mod_i = 0; mod_i < pe->modifiers_size; mod_i++) {
        float t = T1_easing_t_to_eased_t(
            /* const T1TPair t: */
                1.0f,
            /* const T1EasingType easing_type: */
                pe->mods[mod_i].easing_type);
        
        out += pe->mods[mod_i].gpu_stats.xyz[2] * t;
    }
    
    return T1_std_fabs(out);
}

void T1_particle_resize_to_effect_height(
    T1ParticleEffect * to_resize,
    const float new_height)
{
    const float current_height = T1_particle_get_height(to_resize);
    
    log_assert(current_height > 0.0f);
    
    const float multiplier = new_height / current_height;
    
    for (
        uint32_t mod_i = 0;
        mod_i < to_resize->modifiers_size;
        mod_i++)
    {
        for (uint32_t _ = 0; _ < 3; _++) {
            to_resize->mods[mod_i].gpu_stats.xyz[_] *= multiplier;
        }
        to_resize->mods[mod_i].gpu_stats.size *=
            multiplier;
    }
    
    to_resize->base.size *= multiplier;
}

static void T1_particle_add_single_to_frame_data(
    T1GPUFrame * frame_data,
    T1ParticleEffect * pe,
    const uint32_t spawn_i)
{
    if (
        frame_data->
            flat_bb_quads_size + 1 >=
                MAX_TEXQUADS_PER_BUFFER)
    {
        log_assert(0);
        return;
    }
    
    T1GPUFlatQuad * tgt = frame_data->flat_bb_quads + frame_data->flat_bb_quads_size;
    frame_data->flat_bb_quads_size += 1;
    
    *tgt = pe->base;
    
    for (
        uint32_t mod_i = 0;
        mod_i < pe->modifiers_size;
        mod_i++)
    {
        double lifetime_so_far =
            (double)pe->elapsed -
            (double)(pe->pause_per_spawn * spawn_i);
        
        while (lifetime_so_far < 0.0) {
            lifetime_so_far += pe->loop_duration;
        }
        
        lifetime_so_far -= pe->mods[mod_i].start_delay;
        
        if (lifetime_so_far < 0.0) { continue; }
        
        float spawn_t = (float)(
            lifetime_so_far /
            (double)pe->mods[mod_i].duration);
        
        if (spawn_t > 1.0f || spawn_t < 0.0f) { spawn_t = 1.0f;
        }
        
        float * pertime_add_at = (float *)&pe->mods[mod_i].gpu_stats;
        float * recipient_at = (float *)tgt;
        
        float t = T1_easing_t_to_eased_t(
            /* const T1TPair t: */
                spawn_t,
            /* const T1EasingType easing_type: */
                pe->mods[mod_i].easing_type);
        
        float add_pct =
            (float)pe->mods[mod_i].rand_pct_add;
        float sub_pct =
            (float)pe->mods[mod_i].rand_pct_sub;
        float one_percent = 0.01f;
        float hundred_percent = 1.0f;
        
        uint64_t rand_i =
            (pe->random_seed +
                (mod_i << 2) +
                (spawn_i * 9)) %
                (RANDOM_SEQUENCE_SIZE -
                    sizeof(T1GPUFlatQuad));
        
        if (t < 0.0f) { continue; }
        
        SIMD_FLOAT simdf_t = simd_set1_float(t);
        
        for (
            uint32_t j = 0;
            j < (sizeof(T1GPUFlatQuad) / sizeof(float));
            j += SIMD_FLOAT_LANES)
        {
            SIMD_FLOAT fvar_add =
                T1_rand_simd_at_i(
                    rand_i + j);
            fvar_add = simd_mul_floats(
                fvar_add,
                simd_set1_float(add_pct));
            fvar_add = simd_mul_floats(
                fvar_add,
                simd_set1_float(one_percent));
            fvar_add = simd_add_floats(
                fvar_add,
                simd_set1_float(hundred_percent));
            
            SIMD_FLOAT fvar_sub =
                T1_rand_simd_at_i(
                    rand_i + j + 4);
            fvar_sub = simd_mul_floats(
                fvar_sub,
                simd_set1_float(sub_pct));
            fvar_sub = simd_mul_floats(
                fvar_sub,
                simd_set1_float(one_percent));
            fvar_sub = simd_add_floats(
                fvar_sub,
                simd_set1_float(
                    hundred_percent));
            
            SIMD_FLOAT simdf_fullt_add =
                simd_load_floats(
                    (pertime_add_at + j));
            
            // Convert per second values to per us effect
            simdf_fullt_add = simd_mul_floats(
                simdf_fullt_add, simdf_t);
            simdf_fullt_add = simd_mul_floats(
                simdf_fullt_add,
                fvar_add);
            simdf_fullt_add = simd_mul_floats(
                simdf_fullt_add,
                fvar_sub);
            
            SIMD_FLOAT recip =
                simd_load_floats(
                    recipient_at + j);
            recip = simd_add_floats(
                recip, simdf_fullt_add);
            simd_store_floats(
                (recipient_at + j), recip);
        }
    }
    
    float min_size = 0.001f;
    float max_size = 1.0f;
    
    tgt->size =
        ((tgt->size >=  min_size) * tgt->size) +
        ((tgt->size <   min_size) * min_size);
    tgt->size =
        ((tgt->size >= max_size) * max_size) +
        ((tgt->size <  max_size) * tgt->size);
    
    float min_alpha = 0.0f;
    float max_alpha = 1.0f;
    tgt->rgba[3] = T1_std_maxf(tgt->rgba[3], min_alpha);
    tgt->rgba[3] = T1_std_minf(tgt->rgba[3], max_alpha);
}

void T1_particle_add_all_to_frame_data(
    T1GPUFrame * frame_data,
    uint64_t elapsed_us)
{
    // We expect padding to prevent out of bounds
    log_assert(
        sizeof(T1GPUFlatQuad) % sizeof(float) ==
            0);
    log_assert(
        (sizeof(T1GPUFlatQuad) %
            (SIMD_FLOAT_LANES * 4)) == 0);
    
    for (
        uint32_t i = 0;
        i < T1_particle_effects_size;
        i++)
    {
        if (
            T1_particle_effects[i].deleted ||
            !T1_particle_effects[i].committed ||
            frame_data->zsprite_list->size >= MAX_ZSPRITES_PER_BUFFER)
        {
            continue;
        }
        
        T1_particle_effects[i].elapsed += elapsed_us;
        
        if (T1_particle_effects[i].elapsed >
            T1_particle_effects[i].loop_duration)
        {
            if (T1_particle_effects[i].loops == 1) {
                T1_particle_effects[i].deleted = true;
                continue;
            }
            
            if (T1_particle_effects[i].loops > 1) {
                T1_particle_effects[i].loops -= 1;
            }
            
            T1_particle_effects[i].elapsed %=
                T1_particle_effects[i].loop_duration;
        }
        
        for (
            uint32_t spawn_i = 0;
            spawn_i < T1_particle_effects[i].spawns_per_loop;
            spawn_i++)
        {
            #if 0
            uint64_t spawn_at = (spawn_i *
                T1_particle_effects[i].
                    pause_per_spawn);
            
            uint64_t elapsed_since_last_spawn =
                (
                    (T1_particle_effects[i].elapsed >= spawn_at) *
                    (T1_particle_effects[i].elapsed - spawn_at)
                ) +
                (
                    (T1_particle_effects[i].elapsed <  spawn_at) *
                    ((T1_particle_effects[i].elapsed + T1_particle_effects[i].loop_duration) - spawn_at)
                );
            #endif
            
            T1_particle_add_single_to_frame_data(
                frame_data,
                &T1_particle_effects[i],
                spawn_i);
        }
        
        if (T1_particle_effects[i].cast_light) {
            T1GPULight * next = &frame_data->lights[frame_data->postproc_consts->lights_size];
            
            next->angle_xyz[0] = 0.0f;
            next->angle_xyz[1] = 0.0f;
            next->angle_xyz[2] = 0.0f;
            next->rgb[0] =
                T1_particle_effects[i].light_rgb[0];
            next->rgb[1] =
                T1_particle_effects[i].light_rgb[1];
            next->rgb[2] =
                T1_particle_effects[i].light_rgb[2];
            next->reach =
                T1_particle_effects[i].light_reach;
            next->diffuse =
                T1_particle_effects[i].light_strength;
            next->specular =
                T1_particle_effects[i].light_strength *
                    0.15f;
            
            next->xyz[0] =
                T1_particle_effects[i].base.xyz[0];
            next->xyz[1] =
                T1_particle_effects[i].base.xyz[1] + 0.02f;
            next->xyz[2] =
                T1_particle_effects[i].base.xyz[2];
            frame_data->postproc_consts->lights_size += 1;
        }
    }
}

#elif T1_PARTICLES_ACTIVE == T1_INACTIVE
#else
#error
#endif // PARTICLES_ACTIVE
