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


#if 0
#define LINEPARTICLE_EFFECTS_SIZE 2
#define MAX_LINEPARTICLE_DIRECTIONS 5
typedef struct LineParticle {
    T1CPUzSprite zpolygon_cpu;
    T1GPUzSprite zpolygon_gpu;
    
    uint64_t random_seed;
    uint64_t elapsed;
    uint64_t trail_delay;
    uint64_t wait_first;
    
    uint64_t waypoint_duration[MAX_LINEPARTICLE_DIRECTIONS];
    float waypoint_x[MAX_LINEPARTICLE_DIRECTIONS];
    float waypoint_y[MAX_LINEPARTICLE_DIRECTIONS];
    float waypoint_z[MAX_LINEPARTICLE_DIRECTIONS];
    float waypoint_r[MAX_LINEPARTICLE_DIRECTIONS];
    float waypoint_g[MAX_LINEPARTICLE_DIRECTIONS];
    float waypoint_b[MAX_LINEPARTICLE_DIRECTIONS];
    float waypoint_a[MAX_LINEPARTICLE_DIRECTIONS];
    float
        waypoint_scalefactor
            [MAX_LINEPARTICLE_DIRECTIONS];
    uint32_t waypoints_size;
    
    uint64_t particle_zangle_variance_pct;
    uint64_t particle_scalefactor_variance_pct;
    uint64_t particle_rgb_variance_pct;
    uint32_t particle_count;
    
    uint32_t deleted;
    uint32_t committed;
} T1LineParticle;
extern T1LineParticle * T1_particle_lineparticle_effects;
extern uint32_t T1_particle_lineparticle_effects_size;
T1LineParticle * T1_particle_lineparticle_get_next(void);
T1LineParticle * T1_particle_lineparticle_get_next_with_zsprite(
    T1CPUzSprite * construct_with_zpolygon,
    T1GPUzSprite * construct_with_polygon_gpu);
void T1_particle_lineparticle_commit(
    T1LineParticle * to_commit);
void T1_particle_lineparticle_add_all_to_frame_data(
    T1GPUFrame * frame_data,
    uint64_t elapsed_us,
    const bool32_t alpha_blending);
#endif

/*
If "randomize" is set:
-> random floats are generated from 0.0f to 1.0f
-> those numbers are multiplied by the variance multipliers
e.g. if you set one to 0.1f, the final new rand will in [0.0f - 0.1f]
*/
typedef struct {
    T1GPUzSprite          gpu_stats;
    T1CPUzSpriteSimdStats cpu_stats;
    uint64_t              start_delay;
    uint64_t              duration;
    T1EasingType          easing_type;
    uint8_t               random_t_add;
    uint8_t               random_t_sub;
} T1ParticleMod;

#define T1_PARTICLE_MODS_CAP 5
typedef struct {
    T1ParticleMod mods[T1_PARTICLE_MODS_CAP];
    
    T1GPUzSprite zpolygon_gpu;
    T1CPUzSprite zpolygon_cpu;
    
    uint64_t random_seed;
    uint64_t elapsed;
    uint64_t spawn_lifespan;
    uint64_t loop_duration;
    uint64_t pause_per_spawn;
    
    int32_t zsprite_id;
    
    uint32_t spawns_per_loop;
    uint32_t verts_per_particle;
    uint32_t loops; // 0 for infinite loops
    
    float light_reach;
    float light_strength;
    float light_rgb[3];
    
    uint8_t modifiers_size;
    bool8_t shattered;
    bool8_t cast_light;
    bool8_t deleted;
    bool8_t committed;
} T1ParticleEffect;

extern T1ParticleEffect * T1_particle_effects;
extern uint32_t T1_particle_effects_size;

void T1_particle_effect_construct(T1ParticleEffect * to_construct);

T1ParticleEffect * T1_particle_get_next(void);
void T1_particle_commit(T1ParticleEffect * to_commit);

void T1_particle_delete(int32_t with_object_id);

void T1_particle_resize_to_effect_height(
    T1ParticleEffect * to_resize,
    const float new_height);

void T1_particle_add_all_to_frame_data(
    T1GPUFrame * frame_data,
    uint64_t elapsed_us,
    const bool32_t alpha_blending);

void T1_particle_serialize(
    T1ParticleEffect * to_serialize,
    uint8_t * buffer,
    uint32_t * buffer_size);

void T1_particle_deserialize(
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
