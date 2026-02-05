#ifndef T1_PARTICLE_H
#define T1_PARTICLE_H

#if T1_PARTICLES_ACTIVE == T1_ACTIVE

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PARTICLE_TEXTURES 10
#define PARTICLE_RGBA_PROGRESSION_MAX 10

#include "T1_cpu_to_gpu_types.h"
#include "T1_std.h"
#include "T1_logger.h"
#include "T1_random.h"
#include "T1_zsprite.h"
#include "T1_easing.h"
#include "T1_mesh_summary.h"

/*
If "randomize" is set:
-> random floats are generated from 0.0f to 1.0f
-> those numbers are multiplied by the variance multipliers
e.g. if you set one to 0.1f, the final new rand will in [0.0f - 0.1f]
*/
typedef struct {
    T1GPUFlatQuad         gpu_stats;
    uint64_t              start_delay;
    uint64_t              duration;
    T1EasingType          easing_type;
    uint8_t               rand_pct_add;
    uint8_t               rand_pct_sub;
} T1ParticleMod;

#define T1_PARTICLE_MODS_CAP 5
typedef struct {
    T1ParticleMod mods[T1_PARTICLE_MODS_CAP];
    
    T1GPUFlatQuad base;
    
    uint64_t random_seed;
    uint64_t elapsed;
    uint64_t spawn_lifespan;
    uint64_t loop_duration;
    uint64_t pause_per_spawn;
    
    int32_t zsprite_id;
    
    uint32_t spawns_per_loop;
    uint32_t loops; // 0 for infinite loops
    
    float light_reach;
    float light_strength;
    float light_rgb[3];
    
    uint8_t modifiers_size;
    bool8_t cast_light;
    bool8_t deleted;
    bool8_t committed;
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
    int32_t with_object_id);

void
T1_particle_effects_delete_all(void);

void
T1_particle_resize_to_effect_height(
    T1ParticleEffect * to_resize,
    const float new_height);

void
T1_particle_add_all_to_frame_data(
    T1GPUFrame * frame_data,
    uint64_t elapsed_us);

void
T1_particle_serialize(
    T1ParticleEffect * to_serialize,
    uint8_t * buffer,
    uint32_t * buffer_size);

void
T1_particle_deserialize(
    T1ParticleEffect * recipient,
    uint8_t * buffer,
    uint32_t * buffer_size);

#ifdef __cplusplus
}
#endif

#elif T1_PARTICLES_ACTIVE == T1_INACTIVE
#else
#error
#endif // T1_PARTICLES_ACTIVE

#endif // T1_PARTICLE_H
