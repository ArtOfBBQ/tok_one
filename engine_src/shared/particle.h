#ifndef PARTICLE_H
#define PARTICLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "clientlogic_macro_settings.h"
#include "common.h"
#include "logger.h"
#include "zpolygon.h"

typedef struct ParticleEffect {
    int32_t object_id;
    float x;
    float y;
    float z;
    float height; // the maximum height the particles ascend to
    float top_width; // the maximum x and y spread of the particles
    float origin_width; // if this is tiny the particles spread as they go up
    float scale_factor; // scales width/height when generating particles
    uint32_t spawns_per_second;
    float particle_size;
    uint64_t duration_per_particle;
    uint64_t elapsed;
    bool32_t deleted;
    float origin_rgba[4];
    float final_rgba[4];
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
