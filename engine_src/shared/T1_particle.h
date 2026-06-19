#ifndef T1_PARTICLE_H
#define T1_PARTICLE_H

#include "T1_cpu_to_gpu.h"
#include "T1_easing.h"

#if T1_PARTICLES_ACTIVE == T1_ACTIVE

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PARTICLE_TEXTURES 10
#define PARTICLE_RGBA_PROGRESSION_MAX 10

/*
If "randomize" is set:
-> random f32s are generated from 0.0f to 1.0f
-> those numbers are multiplied by the variance multipliers
e.g. if you set one to 0.1f, the final new rand will in [0.0f - 0.1f]
*/
typedef struct {
    T1GPUFlatQuad         gpu_stats;
    u64              start_delay;
    u64              duration;
    T1EasingType          easing_type;
    u8               rand_pct_add;
    u8               rand_pct_sub;
} T1ParticleMod;

#define T1_PARTICLE_MODS_CAP 5
typedef struct {
    T1ParticleMod mods[T1_PARTICLE_MODS_CAP];
    
    T1GPUFlatQuad base;
    
    u64 random_seed;
    u64 elapsed;
    u64 spawn_lifespan;
    u64 loop_duration;
    u64 pause_per_spawn;
    
    s32 T1_id;
    
    u32 spawns_per_loop;
    u32 loops; // 0 for infinite loops
    
    f32 light_reach;
    f32 light_strength;
    f32 light_rgb[3];
    
    u8 modifiers_size;
    u8 cast_light;
    u8 deleted;
    u8 committed;
} T1ParticleEffect;

void
T1_particle_init(void);

void
T1_particle_effect_construct(
    T1ParticleEffect * to_construct);

T1ParticleEffect *
T1_particle_get_next(void);

void
T1_particle_commit(T1ParticleEffect * to_commit);

void
T1_particle_delete(
    s32 with_object_id);

void
T1_particle_effects_delete_all(void);

void
T1_particle_resize_to_effect_height(
    T1ParticleEffect * to_resize,
    const f32 new_height);

void
T1_particle_add_all_to_frame_data(
    T1GPUFrame * frame_data,
    u64 elapsed_us);

void
T1_particle_serialize(
    T1ParticleEffect * to_serialize,
    u8 * buffer,
    u32 * buffer_size);

void
T1_particle_deserialize(
    T1ParticleEffect * recipient,
    u8 * buffer,
    u32 * buffer_size);

#ifdef __cplusplus
}
#endif

#elif T1_PARTICLES_ACTIVE == T1_INACTIVE
#else
#error
#endif // T1_PARTICLES_ACTIVE

#endif // T1_PARTICLE_H
