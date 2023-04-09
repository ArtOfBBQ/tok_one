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
    
    to_construct->particle_spawns_per_second = 200;
    to_construct->particle_size = 0.01f;
    to_construct->random_seed = tok_rand() % 750;
    to_construct->particle_lifespan = 2000000;
    to_construct->elapsed = 0;
    to_construct->deleted = false;
    
    to_construct->particle_origin_rgba[0] = 1.0f;
    to_construct->particle_origin_rgba[1] = 1.0f;
    to_construct->particle_origin_rgba[2] = 1.0f;
    to_construct->particle_origin_rgba[3] = 1.0f;
    
    to_construct->particle_final_rgba[0] = 1.0f;
    to_construct->particle_final_rgba[1] = 1.0f;
    to_construct->particle_final_rgba[2] = 1.0f;
    to_construct->particle_final_rgba[3] = 1.0f;
    
    to_construct->particle_direction[0] = 1.0f;
    to_construct->particle_direction[1] = 0.0f;
    to_construct->particle_direction[2] = 0.0f;
    
    to_construct->particle_distance_per_second = 0.5f;
    
    to_construct->particle_origin_max_x_variance = 0.0f;
    to_construct->particle_origin_max_y_variance = 0.0f;
    to_construct->particle_origin_max_z_variance = 0.0f;
    
    to_construct->particle_direction_max_x_angle_variance = 0;
    to_construct->particle_direction_max_y_angle_variance = 0;
    to_construct->particle_direction_max_z_angle_variance = 0;
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
    zVertex randomized_direction;
    zVertex arbitrary_vector;
    zVertex ray_to_camera;
    
    uint64_t spawns_in_duration;
    uint64_t interval_between_spawns;
    uint64_t spawn_lifetime_so_far;
        
    for (uint32_t i = 0; i < particle_effects_size; i++) {
        if (!particle_effects[i].deleted) {
            
            ray_to_camera.x = particle_effects[i].x - camera.x;
            ray_to_camera.y = particle_effects[i].y - camera.y;
            ray_to_camera.z = particle_effects[i].z - camera.z;
            normalize_zvertex(&ray_to_camera);
            
            // get a vector perpendicular to the camera ray
            arbitrary_vector.x = ray_to_camera.z;
            arbitrary_vector.y = ray_to_camera.x;
            arbitrary_vector.z = ray_to_camera.y;
            
            zVertex triangle_directions[3];
            
            triangle_directions[0] =
                crossproduct_of_zvertices(&ray_to_camera, &arbitrary_vector);
            normalize_zvertex(&triangle_directions[0]);
            // we now have a vector perpendicular to the camera ray
            
            triangle_directions[1] =
                crossproduct_of_zvertices(&arbitrary_vector, &ray_to_camera);
            normalize_zvertex(&triangle_directions[1]);
            
            triangle_directions[2] =
                crossproduct_of_zvertices(&triangle_directions[0], &ray_to_camera);
            normalize_zvertex(&triangle_directions[2]);
            
            particle_effects[i].elapsed += elapsed_nanoseconds;
            particle_effects[i].elapsed =
                particle_effects[i].elapsed %
                    particle_effects[i].particle_lifespan;
            
            spawns_in_duration =
                (particle_effects[i].particle_lifespan / 1000000) *
                    particle_effects[i].particle_spawns_per_second;
            interval_between_spawns =
                1000000 / particle_effects[i].particle_spawns_per_second;
            
            float half_size = particle_effects[i].particle_size * 0.5f;
            
            for (
                uint32_t spawn_i = 0;
                spawn_i < spawns_in_duration;
                spawn_i++)
            {
                uint64_t rand_i =
                    (particle_effects[i].random_seed + (spawn_i * 103)) %
                        RANDOM_SEQUENCE_SIZE;
                
                spawn_lifetime_so_far =
                    (particle_effects[i].elapsed +
                    (spawn_i * interval_between_spawns)) %
                        particle_effects[i].particle_lifespan;
                
                float distance_traveled =
                    ((float)spawn_lifetime_so_far / 1000000.0f) *
                        particle_effects[i].particle_distance_per_second;
                
                randomized_direction.x =
                    particle_effects[i].particle_direction[0];
                randomized_direction.y =
                    particle_effects[i].particle_direction[1];
                randomized_direction.z =
                    particle_effects[i].particle_direction[2];
                
                if (particle_effects[i].
                    particle_direction_max_x_angle_variance > 0)
                {
                    float x_rotation =
                        (float)(tok_rand_at_i(rand_i + 0) %
                            particle_effects[i].
                                particle_direction_max_x_angle_variance) /
                                    100.0f;
                    randomized_direction = x_rotate_zvertex(
                        &randomized_direction,
                        x_rotation);
                }
                
                if (particle_effects[i].
                    particle_direction_max_y_angle_variance > 0)
                {
                    float y_rotation =
                        (float)(tok_rand_at_i(rand_i + 1) %
                            particle_effects[i].
                                particle_direction_max_y_angle_variance) /
                                    100.0f;
                    randomized_direction = y_rotate_zvertex(
                        &randomized_direction,
                        y_rotation);
                }
                
                if (particle_effects[i].
                    particle_direction_max_z_angle_variance > 0)
                {
                    float z_rotation =
                        (float)(tok_rand_at_i(rand_i + 2) %
                            particle_effects[i].
                                particle_direction_max_z_angle_variance) /
                                    100.0f;
                    randomized_direction = z_rotate_zvertex(
                        &randomized_direction,
                        z_rotation);
                }
                
                normalize_zvertex(&randomized_direction);
                
                float fraction_of_duration =
                    (float)spawn_lifetime_so_far /
                        (float)particle_effects[i].particle_lifespan;
                float red =
                    (particle_effects[i].particle_final_rgba[0] *
                        fraction_of_duration) +
                    (particle_effects[i].particle_origin_rgba[0] *
                        (1.0f - fraction_of_duration));
                float green =
                    (particle_effects[i].particle_final_rgba[1] *
                        fraction_of_duration) +
                    (particle_effects[i].particle_origin_rgba[1] *
                        (1.0f - fraction_of_duration));
                float blue =
                    (particle_effects[i].particle_final_rgba[2] *
                        fraction_of_duration) +
                    (particle_effects[i].particle_origin_rgba[2] *
                        (1.0f - fraction_of_duration));
                float alpha =
                    (particle_effects[i].particle_final_rgba[3] *
                        fraction_of_duration) +
                    (particle_effects[i].particle_origin_rgba[3] *
                        (1.0f - fraction_of_duration));
                
                for (uint32_t m = 0; m < 3; m++) {
                    next_gpu_workload[*next_workload_size].parent_x =
                        particle_effects[i].x +
                            (distance_traveled * randomized_direction.x);
                    next_gpu_workload[*next_workload_size].parent_y =
                        particle_effects[i].y +
                            (distance_traveled * randomized_direction.y);
                    next_gpu_workload[*next_workload_size].parent_z =
                        particle_effects[i].z +
                            (distance_traveled * randomized_direction.z);
                    // we're billboarding (always face to camera)
                    next_gpu_workload[*next_workload_size].x =
                        triangle_directions[m].x * half_size;
                    next_gpu_workload[*next_workload_size].y =
                        triangle_directions[m].y * half_size;
                    next_gpu_workload[*next_workload_size].z =
                        triangle_directions[m].z * half_size;
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
