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

typedef struct ShatterEffect {
    zPolygon zpolygon_to_shatter;
    
    uint64_t random_seed;
    uint64_t elapsed;
    
    uint64_t longest_random_delay_before_launch;
    
    uint64_t start_fade_out_at_elapsed;
    uint64_t finish_fade_out_at_elapsed;
    
    float exploding_distance_per_second;
    
    float linear_distance_per_second;
    float linear_direction[3];
    
    float squared_distance_per_second;
    float squared_direction[3];
    
    bool32_t deleted;
    bool32_t committed;
} ShatterEffect;
extern ShatterEffect * shatter_effects;
extern uint32_t shatter_effects_size;

void construct_shatter_effect(
    ShatterEffect * to_construct,
    zPolygon * from_zpolygon);
ShatterEffect * next_shatter_effect(zPolygon * construct_with_zpolygon);
void commit_shatter_effect(
    ShatterEffect * to_commit);

void add_shatter_effects_to_workload(
    GPU_Vertex * next_gpu_workload,
    uint32_t * next_workload_size,
    GPU_LightCollection * lights_for_gpu,
    uint64_t elapsed_nanoseconds);


typedef struct ParticleEffect {
    int32_t object_id;
    
    int32_t mesh_id_to_spawn;
    
    float x;
    float y;
    float z;
    float scale_factor;
    
    uint64_t random_seed;
    uint64_t elapsed;
    bool32_t deleted;
    
    uint32_t particle_spawns_per_second;
    float particle_x_multiplier;
    float particle_y_multiplier;
    float particle_z_multiplier;
    bool32_t particles_ignore_lighting;
    
    uint64_t particle_lifespan;
    uint64_t pause_between_spawns;
    
    float particle_rgba_progression[PARTICLE_RGBA_PROGRESSION_MAX][4];
    uint32_t particle_rgba_progression_size;
    
    float particle_direction[3]; // the direction the particles fly in
    float particle_distance_per_second; // 1.0f to travel 1.0f per second
    // set these to 0 to have each particle fly exactly in particle_direction
    // (so you will basically end up with a line)
    // set max_x_angle_variance to 318 to randomly rotate each particle's
    // direction by somewhere between 0.0f radians and 3.18f radians around the
    // x-axis, so you get particles flying in different directions
    uint32_t particle_direction_max_x_angle_variance;
    uint32_t particle_direction_max_y_angle_variance;
    uint32_t particle_direction_max_z_angle_variance;
    
    float squared_direction[3];
    float squared_distance_per_second;
    uint32_t squared_direction_max_x_angle_variance;
    uint32_t squared_direction_max_y_angle_variance;
    uint32_t squared_direction_max_z_angle_variance;
    
    // set these to 0 to have each particle start exactly at the origin
    // set max_y to 20 to randomly have each particle offset by a y-axis value
    // between 0.0f and 0.2f
    uint32_t particle_origin_max_x_variance;
    uint32_t particle_origin_max_y_variance;
    uint32_t particle_origin_max_z_variance;
    
    int32_t random_texturearray_i[MAX_PARTICLE_TEXTURES];
    int32_t random_texture_i[MAX_PARTICLE_TEXTURES];
    int32_t random_textures_size;
    
    bool32_t generate_light;
    float light_reach;
    float light_strength;
    float light_rgb[3];
} ParticleEffect;

extern ParticleEffect * particle_effects;
extern uint32_t particle_effects_size;

void construct_particle_effect(ParticleEffect * to_construct);

void request_particle_effect(ParticleEffect * to_request);

void delete_particle_effect(int32_t with_object_id);

void add_particle_effects_to_workload(
    GPU_Vertex * next_gpu_workload,
    uint32_t * next_workload_size,
    GPU_LightCollection * lights_for_gpu,
    uint64_t elapsed_nanoseconds);

#ifdef __cplusplus
}
#endif

#endif // PARTICLE_H
