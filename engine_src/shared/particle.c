#include "particle.h"

ParticleEffect * particle_effects;
uint32_t particle_effects_size;

static const float left_uv_coord   = 0.0f;
static const float right_uv_coord  = 1.0f;
static const float bottom_uv_coord = 1.0f;
static const float top_uv_coord    = 0.0f;

void construct_particle_effect(
    ParticleEffect * to_construct)
{
    to_construct->object_id = -1;
    to_construct->mesh_id_to_spawn = 1;
    to_construct->x = 0;
    to_construct->y = 0;
    to_construct->z = 0;
    to_construct->scale_factor = 1.0f;
    
    to_construct->particle_spawns_per_second = 200;
    to_construct->particle_x_multiplier = 0.01f;
    to_construct->particle_y_multiplier = 0.01f;
    to_construct->particle_z_multiplier = 0.01f;
    to_construct->particles_ignore_lighting = true;
    to_construct->random_seed = tok_rand() % 750;
    to_construct->particle_lifespan = 2000000;
    to_construct->pause_between_spawns = 0;
    to_construct->elapsed = 0;
    to_construct->deleted = false;
    
    to_construct->particle_rgba_progression[0][0] = 1.0f;
    to_construct->particle_rgba_progression[0][1] = 1.0f;
    to_construct->particle_rgba_progression[0][2] = 1.0f;
    to_construct->particle_rgba_progression[0][3] = 1.0f;
    
    to_construct->particle_rgba_progression[1][0] = 1.0f;
    to_construct->particle_rgba_progression[1][1] = 1.0f;
    to_construct->particle_rgba_progression[1][2] = 1.0f;
    to_construct->particle_rgba_progression[1][3] = 1.0f;
    
    to_construct->particle_rgba_progression_size = 2;
    
    to_construct->random_textures_size = 0;
    
    to_construct->particle_direction[0] = 1.0f;
    to_construct->particle_direction[1] = 0.0f;
    to_construct->particle_direction[2] = 0.0f;
    to_construct->particle_distance_per_second = 0.5f;
    to_construct->particle_direction_max_x_angle_variance = 0;
    to_construct->particle_direction_max_y_angle_variance = 0;
    to_construct->particle_direction_max_z_angle_variance = 0;
    
    to_construct->squared_direction[0] = 0.0f;
    to_construct->squared_direction[1] = 0.0f;
    to_construct->squared_direction[2] = 0.0f;
    to_construct->squared_distance_per_second = 0.0f;
    to_construct->squared_direction_max_x_angle_variance = 0.0f;
    to_construct->squared_direction_max_y_angle_variance = 0.0f;
    to_construct->squared_direction_max_z_angle_variance = 0.0f;
    
    to_construct->particle_origin_max_x_variance = 0.0f;
    to_construct->particle_origin_max_y_variance = 0.0f;
    to_construct->particle_origin_max_z_variance = 0.0f;
    
    to_construct->generate_light = true;
    to_construct->light_reach = 1.0f;
    to_construct->light_strength = 1.0f;
    to_construct->light_rgb[0] = 1.0f;
    to_construct->light_rgb[1] = 0.5f;
    to_construct->light_rgb[2] = 0.25f;
}

void request_particle_effect(
    ParticleEffect * to_request)
{
    log_assert(
        to_request->particle_rgba_progression_size <=
            PARTICLE_RGBA_PROGRESSION_MAX);
    
    log_assert(to_request->mesh_id_to_spawn >= 0);
    log_assert(to_request->mesh_id_to_spawn < all_mesh_summaries_size);
    
    for (
        uint32_t col_i = 0;
        col_i < to_request->particle_rgba_progression_size;
        col_i++)
    {
        log_assert(to_request->particle_rgba_progression[col_i][0] >= 0.0f);
        log_assert(to_request->particle_rgba_progression[col_i][0] <= 1.0f);
        log_assert(to_request->particle_rgba_progression[col_i][1] >= 0.0f);
        log_assert(to_request->particle_rgba_progression[col_i][1] <= 1.0f);
        log_assert(to_request->particle_rgba_progression[col_i][2] >= 0.0f);
        log_assert(to_request->particle_rgba_progression[col_i][2] <= 1.0f);
        log_assert(to_request->particle_rgba_progression[col_i][3] >= 0.0f);
        log_assert(to_request->particle_rgba_progression[col_i][3] <= 1.0f);
    }
    
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

static void get_particle_color_at_elapsed(
    const uint32_t particle_i,
    const uint64_t elapsed,
    float * out_red,
    float * out_green,
    float * out_blue,
    float * out_alpha)
{
    // Get the current color (which is somewhere between origin_rgba
    // and final_rgba)
    uint64_t single_color_duration =
        (particle_effects[particle_i].particle_lifespan /
            particle_effects[particle_i].particle_rgba_progression_size);
    
    uint32_t first_color_i =
        (uint32_t)(elapsed /
            (single_color_duration + 5));
    
    log_assert(first_color_i >= 0);
    if (
        first_color_i >=
            particle_effects[particle_i].particle_rgba_progression_size)
    {
        first_color_i =
            particle_effects[particle_i].particle_rgba_progression_size - 1;
    }
    log_assert(
        first_color_i <
            particle_effects[particle_i].particle_rgba_progression_size);
    
    uint32_t second_color_i =
        first_color_i + 1 <
            particle_effects[particle_i].particle_rgba_progression_size ?
        first_color_i + 1 :
        first_color_i;
    
    float transition_progress =
        (float)(elapsed % single_color_duration) /
            1000000.0f;
    log_assert(transition_progress >= 0.0f);
    log_assert(transition_progress <= 1.0f);
    
    *out_red =
        (particle_effects[particle_i].
            particle_rgba_progression[first_color_i][0] *
                (1.0f - transition_progress)) +
        (particle_effects[particle_i].
            particle_rgba_progression[second_color_i][0] *
                transition_progress);
    *out_green =
        (particle_effects[particle_i].
            particle_rgba_progression[first_color_i][1] *
                (1.0f - transition_progress)) +
        (particle_effects[particle_i].
            particle_rgba_progression[second_color_i][1] *
                transition_progress);
    *out_blue =
        (particle_effects[particle_i].
            particle_rgba_progression[first_color_i][2] *
                (1.0f - transition_progress)) +
        (particle_effects[particle_i].
            particle_rgba_progression[second_color_i][2] *
                transition_progress);
    
    if (out_alpha == NULL) { return; }
    
    *out_alpha =
        (particle_effects[particle_i].
            particle_rgba_progression[first_color_i][3] *
                (1.0f - transition_progress)) +
        (particle_effects[particle_i].
            particle_rgba_progression[second_color_i][3] *
                transition_progress);
}

void add_particle_effects_to_workload(
    GPU_Vertex * next_gpu_workload,
    uint32_t * next_workload_size,
    GPU_LightCollection * lights_for_gpu,
    uint64_t elapsed_nanoseconds)
{
    zVertex randomized_direction;
    zVertex randomized_squared_direction;
    
    uint64_t spawns_in_duration;
    uint64_t interval_between_spawns;
    uint64_t spawn_lifetime_so_far;
        
    for (uint32_t i = 0; i < particle_effects_size; i++) {
        if (!particle_effects[i].deleted) {
                        
            log_assert(
               particle_effects[i].particle_rgba_progression_size <=
                   PARTICLE_RGBA_PROGRESSION_MAX);
            
            particle_effects[i].elapsed += elapsed_nanoseconds;
            particle_effects[i].elapsed =
                particle_effects[i].elapsed %
                    (particle_effects[i].particle_lifespan +
                        particle_effects[i].pause_between_spawns);
            
            spawns_in_duration =
                (particle_effects[i].particle_lifespan / 1000000) *
                    particle_effects[i].particle_spawns_per_second;
            interval_between_spawns =
                1000000 / particle_effects[i].particle_spawns_per_second;
            
            uint32_t particles_active = 0;
            
            for (
                uint32_t spawn_i = 0;
                spawn_i < spawns_in_duration;
                spawn_i++)
            {
                uint64_t rand_i =
                    (particle_effects[i].random_seed + (spawn_i * 13)) %
                        (RANDOM_SEQUENCE_SIZE - 50);
                
                int32_t texturearray_i = -1;
                int32_t texture_i = -1;
                
                if (particle_effects[i].random_textures_size > 0) {
                    int32_t rand_texture_i =
                        (int32_t)tok_rand_at_i(rand_i + 12) %
                            particle_effects[i].random_textures_size;
                    
                    texturearray_i = particle_effects[i].
                        random_texturearray_i[rand_texture_i];
                    texture_i = particle_effects[i].
                        random_texture_i[rand_texture_i];
                }
                
                spawn_lifetime_so_far =
                    (particle_effects[i].elapsed +
                    (spawn_i * interval_between_spawns)) %
                        (particle_effects[i].particle_lifespan +
                            particle_effects[i].pause_between_spawns);
                
                if (spawn_lifetime_so_far >
                    particle_effects[i].particle_lifespan)
                {
                    continue;
                }
                
                particles_active += 1;
                
                float distance_traveled =
                    ((float)spawn_lifetime_so_far / 1000000.0f) *
                        particle_effects[i].particle_distance_per_second;
                float sq_distance_traveled =
                    (((float)spawn_lifetime_so_far / 1000000.0f) *
                    ((float)spawn_lifetime_so_far / 1000000.0f)) *
                        particle_effects[i].squared_distance_per_second;
                
                randomized_direction.x =
                    particle_effects[i].particle_direction[0];
                randomized_direction.y =
                    particle_effects[i].particle_direction[1];
                randomized_direction.z =
                    particle_effects[i].particle_direction[2];
                randomized_squared_direction.x =
                    particle_effects[i].squared_direction[0];
                randomized_squared_direction.y =
                    particle_effects[i].squared_direction[1];
                randomized_squared_direction.z =
                    particle_effects[i].squared_direction[2];
                
                if (particle_effects[i].
                    particle_direction_max_x_angle_variance > 0)
                {
                    float x_rotation_pos = (float)(
                        tok_rand_at_i(rand_i + 0) %
                            particle_effects[i].
                                particle_direction_max_x_angle_variance) /
                                    100.0f;
                    float x_rotation_neg = (float)(
                        tok_rand_at_i(rand_i + 1) %
                            particle_effects[i].
                                particle_direction_max_x_angle_variance) /
                                    100.0f;
                    
                    float x_rotation = x_rotation_pos - x_rotation_neg;
                    randomized_direction = x_rotate_zvertex(
                        &randomized_direction,
                        x_rotation);
                }
                
                if (particle_effects[i].
                    squared_direction_max_x_angle_variance > 0)
                {
                    float x_rotation_pos = (float)(
                        tok_rand_at_i(rand_i + 14) %
                            particle_effects[i].
                                squared_direction_max_x_angle_variance) /
                                    100.0f;
                    float x_rotation_neg = (float)(
                        tok_rand_at_i(rand_i + 15) %
                            particle_effects[i].
                                squared_direction_max_x_angle_variance) /
                                    100.0f;
                    
                    float x_rotation = x_rotation_pos - x_rotation_neg;
                    
                    randomized_squared_direction = x_rotate_zvertex(
                        &randomized_squared_direction,
                        x_rotation);
                }
                
                if (particle_effects[i].
                    particle_direction_max_y_angle_variance > 0)
                {
                    float y_rotation_pos = (float)(
                        tok_rand_at_i(rand_i + 2) %
                            particle_effects[i].
                                particle_direction_max_y_angle_variance) /
                                    100.0f;
                    float y_rotation_neg = (float)(
                        tok_rand_at_i(rand_i + 3) %
                            particle_effects[i].
                                particle_direction_max_y_angle_variance) /
                                    100.0f;
                    float y_rotation = y_rotation_pos - y_rotation_neg;
                    randomized_direction = y_rotate_zvertex(
                        &randomized_direction,
                        y_rotation);
                }
                
                if (particle_effects[i].
                    squared_direction_max_y_angle_variance > 0)
                {
                    float y_rotation_pos = (float)(
                        tok_rand_at_i(rand_i + 14) %
                            particle_effects[i].
                                squared_direction_max_y_angle_variance) /
                                    100.0f;
                    float y_rotation_neg = (float)(
                        tok_rand_at_i(rand_i + 15) %
                            particle_effects[i].
                                squared_direction_max_y_angle_variance) /
                                    100.0f;
                    
                    float y_rotation = y_rotation_pos - y_rotation_neg;
                    randomized_squared_direction = y_rotate_zvertex(
                        &randomized_squared_direction,
                        y_rotation);
                }
                
                if (particle_effects[i].
                    particle_direction_max_z_angle_variance > 0)
                {
                    float z_rotation_pos = (float)(
                        tok_rand_at_i(rand_i + 4) %
                            particle_effects[i].
                                particle_direction_max_z_angle_variance) /
                                    100.0f;
                    float z_rotation_neg = (float)(
                        tok_rand_at_i(rand_i + 5) %
                            particle_effects[i].
                                particle_direction_max_z_angle_variance) /
                                    100.0f;
                    float z_rotation = z_rotation_pos - z_rotation_neg;
                    randomized_direction = z_rotate_zvertex(
                        &randomized_direction,
                        z_rotation);
                }
                
                if (particle_effects[i].
                    squared_direction_max_z_angle_variance > 0)
                {
                    float z_rotation_pos = (float)(
                        tok_rand_at_i(rand_i + 14) %
                            particle_effects[i].
                                squared_direction_max_z_angle_variance) /
                                    100.0f;
                    float z_rotation_neg = (float)(
                        tok_rand_at_i(rand_i + 15) %
                            particle_effects[i].
                                squared_direction_max_z_angle_variance) /
                                    100.0f;
                    
                    float z_rotation = z_rotation_pos - z_rotation_neg;
                    randomized_squared_direction = z_rotate_zvertex(
                        &randomized_squared_direction,
                        z_rotation);
                }
                
                normalize_zvertex(&randomized_direction);
                
                float red;
                float green;
                float blue;
                float alpha;
                get_particle_color_at_elapsed(
                    /* at_particle_i: */
                        i,
                    /* elapsed: */
                        spawn_lifetime_so_far,
                    /* const float * out_red: */
                        &red,
                    /* const float * out_green: */
                        &green,
                    /* const float * out_blue: */
                        &blue,
                    /* const float * alpha: */
                        &alpha);
                
                float initial_x_offset = 0;
                if (particle_effects[i].particle_origin_max_x_variance > 0)
                {
                    float x_offset_pos = (float)(
                        tok_rand_at_i(rand_i + 6) %
                            particle_effects[i].
                                particle_origin_max_x_variance) /
                                    100.0f;
                    float x_offset_neg = (float)(
                        tok_rand_at_i(rand_i + 7) %
                            particle_effects[i].
                                particle_origin_max_x_variance) /
                                    100.0f;
                    initial_x_offset += (x_offset_pos - x_offset_neg);
                }
                
                float initial_y_offset = 0;
                if (particle_effects[i].particle_origin_max_y_variance > 0)
                {
                    float y_offset_pos = (float)(
                        tok_rand_at_i(rand_i + 8) %
                            particle_effects[i].
                                particle_origin_max_y_variance) /
                                    100.0f;
                    float y_offset_neg = (float)(
                        tok_rand_at_i(rand_i + 9) %
                            particle_effects[i].
                                particle_origin_max_x_variance) /
                                    100.0f;
                    initial_y_offset += (y_offset_pos - y_offset_neg);
                }
                
                float initial_z_offset = 0;
                if (particle_effects[i].particle_origin_max_z_variance > 0)
                {
                    float z_offset_pos = (float)(
                        tok_rand_at_i(rand_i + 10) %
                            particle_effects[i].
                                particle_origin_max_z_variance) /
                                    100.0f;
                    float z_offset_neg = (float)(
                        tok_rand_at_i(rand_i + 11) %
                            particle_effects[i].
                                particle_origin_max_z_variance) /
                                    100.0f;
                    initial_z_offset += (z_offset_pos - z_offset_neg);
                }
                
                for (
                    int32_t tri_i = all_mesh_summaries[
                        particle_effects[i].mesh_id_to_spawn].all_meshes_head_i;
                    tri_i <
                        all_mesh_summaries[
                            particle_effects[i].mesh_id_to_spawn].
                                all_meshes_head_i +
                        all_mesh_summaries[
                            particle_effects[i].mesh_id_to_spawn].
                                triangles_size;
                    tri_i++)
                {
                    log_assert(tri_i >= 0);
                    log_assert(tri_i < (int32_t)all_mesh_triangles_size);
                    
                    for (int32_t m = 0; m < 3; m++) {
                        next_gpu_workload[*next_workload_size].parent_x =
                            (particle_effects[i].x + initial_x_offset) +
                                (distance_traveled * randomized_direction.x) +
                                (sq_distance_traveled *
                                    randomized_squared_direction.x);
                        next_gpu_workload[*next_workload_size].parent_y =
                            (particle_effects[i].y + initial_y_offset) +
                                (distance_traveled * randomized_direction.y) +
                                (sq_distance_traveled *
                                    randomized_squared_direction.y);
                        next_gpu_workload[*next_workload_size].parent_z =
                            (particle_effects[i].z + initial_z_offset) +
                                (distance_traveled * randomized_direction.z) +
                                (sq_distance_traveled *
                                    randomized_squared_direction.z);
                        
                        next_gpu_workload[*next_workload_size].x =
                            all_mesh_triangles[tri_i].vertices[m].x *
                                particle_effects[i].particle_x_multiplier;
                        next_gpu_workload[*next_workload_size].y =
                            all_mesh_triangles[tri_i].vertices[m].y *
                                particle_effects[i].particle_y_multiplier;;
                        next_gpu_workload[*next_workload_size].z =
                            all_mesh_triangles[tri_i].vertices[m].z *
                                particle_effects[i].particle_z_multiplier;;
                        next_gpu_workload[*next_workload_size].uv[0] =
                            all_mesh_triangles[tri_i].vertices[m].uv[0];
                        next_gpu_workload[*next_workload_size].uv[1] =
                            all_mesh_triangles[tri_i].vertices[m].uv[1];
                        next_gpu_workload[*next_workload_size].texturearray_i =
                            texturearray_i;
                        next_gpu_workload[*next_workload_size].texture_i =
                            texture_i;
                        next_gpu_workload[*next_workload_size].normal_x =
                            all_mesh_triangles[tri_i].normal.x;
                        next_gpu_workload[*next_workload_size].normal_y =
                            all_mesh_triangles[tri_i].normal.y;
                        next_gpu_workload[*next_workload_size].normal_z =
                            all_mesh_triangles[tri_i].normal.z;
                        next_gpu_workload[*next_workload_size].ignore_lighting =
                            particle_effects[i].particles_ignore_lighting;
                        next_gpu_workload[*next_workload_size].ignore_camera =
                            false;
                        next_gpu_workload[*next_workload_size].scale_factor =
                            1.0f;
                        next_gpu_workload[*next_workload_size].touchable_id =
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
            
            if (particles_active < 1) {
                particle_effects[i].random_seed =
                    tok_rand() % RANDOM_SEQUENCE_SIZE;
            } else if (particle_effects[i].generate_light) {
                lights_for_gpu->light_x[lights_for_gpu->lights_size] =
                    particle_effects[i].x;
                lights_for_gpu->light_y[lights_for_gpu->lights_size] =
                    particle_effects[i].y;
                lights_for_gpu->light_z[lights_for_gpu->lights_size] =
                    particle_effects[i].z;
                
                lights_for_gpu->red[lights_for_gpu->lights_size] =
                    particle_effects[i].light_rgb[0];
                lights_for_gpu->green[lights_for_gpu->lights_size] =
                    particle_effects[i].light_rgb[1];
                lights_for_gpu->blue[lights_for_gpu->lights_size] =
                    particle_effects[i].light_rgb[2];
                
                lights_for_gpu->reach[lights_for_gpu->lights_size] =
                    particle_effects[i].light_reach;
                
                float light_strength =
                    particle_effects[i].light_strength * (
                        (float)particles_active / (float)spawns_in_duration);
                
                lights_for_gpu->ambient[lights_for_gpu->lights_size] =
                    0.05f * light_strength;
                lights_for_gpu->diffuse[lights_for_gpu->lights_size] =
                    1.0f * light_strength;
                lights_for_gpu->lights_size += 1;
            }
        }
    }
}