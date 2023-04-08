#include "particle.h"

ParticleEffect * particle_effects;
uint32_t particle_effects_size;

void construct_particle_effect(
    ParticleEffect * to_construct)
{
    to_construct->object_id = -1;
    to_construct->x = 0;
    to_construct->y = 0;
    to_construct->z = 0;
    to_construct->scale_factor = 1.0f;
    to_construct->top_width = 0.02f;
    to_construct->origin_width = 0.02f;
    to_construct->height = 0.02f;
    to_construct->spawns_per_second = 200;
    to_construct->particle_size = 0.01f;
    to_construct->elapsed = 0;
    to_construct->duration_per_particle = 2000000;
    to_construct->deleted = false;
    
    to_construct->origin_rgba[0] = 1.0f;
    to_construct->origin_rgba[1] = 1.0f;
    to_construct->origin_rgba[2] = 1.0f;
    to_construct->origin_rgba[3] = 1.0f;
    
    to_construct->final_rgba[0] = 1.0f;
    to_construct->final_rgba[1] = 1.0f;
    to_construct->final_rgba[2] = 1.0f;
    to_construct->final_rgba[3] = 1.0f;
}

void request_particle_effect(
    ParticleEffect * to_request)
{
    for (uint32_t i = 0; i < particle_effects_size; i++) {
        if (particle_effects[i].deleted) {
            particle_effects[i] = *to_request;
            return;
        }
    }
    
    log_assert(particle_effects_size < PARTICLE_EFFECTS_SIZE);
    particle_effects[particle_effects_size++] = *to_request;
}

void delete_particle_effect(int32_t with_object_id) {
    for (uint32_t i = 0; i < particle_effects_size; i++) {
        if (particle_effects[i].object_id == with_object_id) {
            particle_effects[i].deleted = true;
        }
    }
    
    while (
        particle_effects_size > 0 &&
        particle_effects[particle_effects_size - 1].deleted)
    {
        particle_effects_size -= 1;
    }
}

void add_particle_effects_to_workload(
    GPU_Vertex * next_gpu_workload,
    uint32_t * next_workload_size,
    uint64_t elapsed_nanoseconds)
{
    for (uint32_t i = 0; i < particle_effects_size; i++) {
        if (!particle_effects[i].deleted) {
            
            zVertex ray_to_camera;
            ray_to_camera.x = particle_effects[i].x - camera.x;
            ray_to_camera.y = particle_effects[i].y - camera.y;
            ray_to_camera.z = particle_effects[i].z - camera.z;
            normalize_zvertex(&ray_to_camera);
            
            particle_effects[i].elapsed += elapsed_nanoseconds;
            particle_effects[i].elapsed =
                particle_effects[i].elapsed %
                    particle_effects[i].duration_per_particle;
            
            uint64_t spawns_in_duration =
                (particle_effects[i].duration_per_particle / 1000000) *
                    particle_effects[i].spawns_per_second;
            uint64_t interval_between_spawns =
                1000000 / particle_effects[i].spawns_per_second;
            
            float half_size = particle_effects[i].particle_size * 0.5f;
            
            for (
                uint32_t spawn_i = 0;
                spawn_i < spawns_in_duration;
                spawn_i++)
            {
                uint64_t spawn_lifetime_so_far =
                    (particle_effects[i].elapsed +
                    (spawn_i * interval_between_spawns)) %
                        particle_effects[i].duration_per_particle;
                
                float xy_direction_radians =
                    ((spawn_i * 100000) % 6280) / 1000.0f;
                float xy_distance_traveled =
                    (
                        (((particle_effects[i].top_width /
                            particle_effects[i].duration_per_particle) *
                                spawn_lifetime_so_far)) +
                        particle_effects[i].origin_width)
                    / 2;
                
                float x_offset =
                    sinf(xy_direction_radians) * xy_distance_traveled;
                float y_offset =
                    cosf(xy_direction_radians) * xy_distance_traveled;
                float z_offset = -((particle_effects[i].height /
                    particle_effects[i].duration_per_particle) *
                        spawn_lifetime_so_far);
                
                float fraction_of_duration =
                    (float)spawn_lifetime_so_far /
                        (float)particle_effects[i].duration_per_particle;
                float red =
                    (particle_effects[i].final_rgba[0] *
                        fraction_of_duration) +
                    (particle_effects[i].origin_rgba[0] *
                        (1.0f - fraction_of_duration));
                float green =
                    (particle_effects[i].final_rgba[1] *
                        fraction_of_duration) +
                    (particle_effects[i].origin_rgba[1] *
                        (1.0f - fraction_of_duration));
                float blue =
                    (particle_effects[i].final_rgba[2] *
                        fraction_of_duration) +
                    (particle_effects[i].origin_rgba[2] *
                        (1.0f - fraction_of_duration));
                float alpha =
                    (particle_effects[i].final_rgba[3] *
                        fraction_of_duration) +
                    (particle_effects[i].origin_rgba[3] *
                        (1.0f - fraction_of_duration));
                
                for (uint32_t m = 0; m < 3; m++) {
                    next_gpu_workload[*next_workload_size].parent_x =
                        particle_effects[i].x + x_offset;
                    next_gpu_workload[*next_workload_size].parent_y =
                        particle_effects[i].y + y_offset;
                    next_gpu_workload[*next_workload_size].parent_z =
                        particle_effects[i].z + z_offset;
                    next_gpu_workload[*next_workload_size].x =
                        m < 1 ?
                            -half_size :
                            half_size;
                    next_gpu_workload[*next_workload_size].y =
                        m < 2 ?
                            -half_size :
                            half_size;
                    next_gpu_workload[*next_workload_size].z = 0.0f;
                    // billboarding (always face to camera)
                    next_gpu_workload[*next_workload_size].normal_x =
                        ray_to_camera.x;
                    next_gpu_workload[*next_workload_size].normal_y =
                        ray_to_camera.y;
                    next_gpu_workload[*next_workload_size].normal_z =
                        ray_to_camera.z;
                    next_gpu_workload[*next_workload_size].ignore_lighting =
                        true;
                    next_gpu_workload[*next_workload_size].ignore_camera =
                        false;
                    next_gpu_workload[*next_workload_size].scale_factor =
                        1.0f;
                    next_gpu_workload[*next_workload_size].touchable_id =
                        -1;
                    next_gpu_workload[*next_workload_size].texture_i = -1;
                    next_gpu_workload[*next_workload_size].texturearray_i =
                        -1;
                    next_gpu_workload[*next_workload_size].x_angle = 0.0f;
                    next_gpu_workload[*next_workload_size].y_angle = 0.0f;
                    next_gpu_workload[*next_workload_size].z_angle = 0.0f;
                    next_gpu_workload[*next_workload_size].RGBA[0] = red;
                    next_gpu_workload[*next_workload_size].RGBA[1] = green;
                    next_gpu_workload[*next_workload_size].RGBA[2] = blue;
                    next_gpu_workload[*next_workload_size].RGBA[3] = alpha;
                    
                    *next_workload_size += 1;
                }
            }
        }
    }
}
