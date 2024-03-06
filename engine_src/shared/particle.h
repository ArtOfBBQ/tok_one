#ifndef PARTICLE_H
#define PARTICLE_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PARTICLE_TEXTURES 10
#define PARTICLE_RGBA_PROGRESSION_MAX 10

#include "cpu_to_gpu_types.h"
#include "clientlogic_macro_settings.h"
#include "common.h"
#include "logger.h"
#include "tok_random.h"
#include "zpolygon.h"

#define LINEPARTICLE_EFFECTS_SIZE 100
#define MAX_LINEPARTICLE_DIRECTIONS 5
typedef struct LineParticle {
    zPolygonCPU zpolygon_cpu;
    GPUPolygon zpolygon_gpu;
    GPUPolygonMaterial zpolygon_material;
    
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
    float waypoint_scalefactor[MAX_LINEPARTICLE_DIRECTIONS];
    uint32_t waypoints_size;
    
    uint64_t particle_zangle_variance_pct;
    uint64_t particle_scalefactor_variance_pct;
    uint64_t particle_rgb_variance_pct;
    uint32_t particle_count;
    
    uint32_t deleted;
    uint32_t committed;
} LineParticle;
extern LineParticle * lineparticle_effects;
extern uint32_t lineparticle_effects_size;
LineParticle * next_lineparticle_effect(void);
LineParticle * next_lineparticle_effect_with_zpoly(
    zPolygonCPU * construct_with_zpolygon,
    GPUPolygon * construct_with_polygon_gpu,
    GPUPolygonMaterial * construct_with_polygon_material);
void commit_lineparticle_effect(
    LineParticle * to_commit);
void add_lineparticle_effects_to_workload(
    GPUDataForSingleFrame * frame_data,
    uint64_t elapsed_nanoseconds);

typedef struct ParticleEffect {
    GPUPolygon gpustats_initial_random_add_1;
    GPUPolygon gpustats_initial_random_add_2;
    GPUPolygon gpustats_pertime_add;
    GPUPolygon gpustats_pertime_random_add_1;
    GPUPolygon gpustats_pertime_random_add_2;
    GPUPolygon gpustats_perexptime_add;
    
    // Reminder on the way the linear variance multipliers work:
    // -> random floats are generated from 0.0f to 1.0f
    // -> those numbers are multiplied by the variance multipliers
    //    e.g. if you set one to 0.1f, the final new rand will in [0.0f - 0.1f]
    // -> each property adds to itself (rand * self)
    // -> each property subtracts from itself (rand2 * self)
    // You now have a property with some variance introduced. Most will center
    // around the original value and the exceedingly rare case will be
    // (linear_variance_multiplier * self) below or above its original value
    
    zPolygonCPU zpolygon_cpu;
    GPUPolygon zpolygon_gpu;
    GPUPolygonMaterial zpolygon_material;
    
    int32_t object_id;
    
    uint64_t random_seed;
    uint64_t elapsed;
    bool32_t deleted;
    bool32_t committed;
    
    int32_t random_texturearray_i[MAX_PARTICLE_TEXTURES];
    int32_t random_texture_i[MAX_PARTICLE_TEXTURES];
    uint32_t random_textures_size;
    
    uint32_t particle_spawns_per_second;
    uint32_t vertices_per_particle;
    uint32_t loops; // 0 for infinite loops
    uint64_t particle_lifespan;
    uint64_t pause_between_spawns;
    
    bool32_t use_shattered_mesh;
    bool32_t generate_light;
    float light_reach;
    float light_strength;
    float light_rgb[3];
} ParticleEffect;

extern ParticleEffect * particle_effects;
extern uint32_t particle_effects_size;

void construct_particle_effect(ParticleEffect * to_construct);

ParticleEffect * next_particle_effect(void);
void commit_particle_effect(ParticleEffect * to_commit);

void delete_particle_effect(int32_t with_object_id);

void add_particle_effects_to_workload(
    GPUDataForSingleFrame * frame_data,
    uint64_t elapsed_nanoseconds);

#ifdef __cplusplus
}
#endif

#endif // PARTICLE_H
