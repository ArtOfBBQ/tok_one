#ifndef PARTICLE_H
#define PARTICLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "clientlogic_macro_settings.h"
#include "common.h"
#include "logger.h"
#include "tok_random.h"
#include "zpolygon.h"

typedef struct ParticleEffect {
    int32_t object_id;
    float x;
    float y;
    float z;
    float scale_factor;
    
    uint64_t random_seed;
    uint64_t elapsed;
    bool32_t deleted;
    
    uint32_t particle_spawns_per_second;
    float particle_size;
    uint64_t particle_lifespan;
    
    float particle_origin_rgba[4];
    float particle_final_rgba[4];
    
    float particle_direction[3]; // the direction the particles fly in
    float particle_distance_per_second; // 1.0f to travel 1.0f per second
    
    // set these to 0 to have each particle start exactly at the origin
    // set max_y to 20 to randomly have each particle offset by a y-axis value
    // between 0.0f and 0.2f
    uint32_t particle_origin_max_x_variance;
    uint32_t particle_origin_max_y_variance;
    uint32_t particle_origin_max_z_variance;
    
    // set these to 0 to have each particle fly exactly in particle_direction
    // (so you will basically end up with a line)
    // set max_x_angle_variance to 318 to randomly rotate each particle's
    // direction by somewhere between 0.0f radians and 3.18f radians around the
    // x-axis, so you get particles flying in different directions
    uint32_t particle_direction_max_x_angle_variance;
    uint32_t particle_direction_max_y_angle_variance;
    uint32_t particle_direction_max_z_angle_variance;
} ParticleEffect;

extern ParticleEffect * particle_effects;
extern uint32_t particle_effects_size;

void construct_particle_effect(ParticleEffect * to_construct);

void request_particle_effect(ParticleEffect * to_request);

void delete_particle_effect(int32_t with_object_id);

void add_particle_effects_to_workload(
    GPU_Vertex * next_gpu_workload,
    uint32_t * next_workload_size,
    uint64_t elapsed_nanoseconds);

#ifdef __cplusplus
}
#endif

#endif // PARTICLE_H
