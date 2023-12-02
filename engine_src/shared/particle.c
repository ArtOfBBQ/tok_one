#include "particle.h"

#define MINIMUM_SHATTER_TRIANGLES 200

ShatterEffect * shatter_effects;
uint32_t shatter_effects_size;

static void construct_shatter_effect_no_zpoly(
    ShatterEffect * to_construct)
{
    to_construct->random_seed =
        tok_rand_at_i(
            platform_get_current_time_microsecs() %
                RANDOM_SEQUENCE_SIZE) % 75;
    to_construct->elapsed = 0;
    to_construct->wait_first = 0;
    to_construct->committed = false;
    to_construct->deleted = false;
    to_construct->limit_triangles_to = 0;
    
    to_construct->longest_random_delay_before_launch = 500000;
    
    //                                         2...000 (2 seconds)
    to_construct->start_fade_out_at_elapsed =  2000000;
    //                                         35..000 (3.5 seconds)
    to_construct->finish_fade_out_at_elapsed = 3500000;
    
    to_construct->exploding_distance_per_second =
        0.25f + (tok_rand_at_i(
            platform_get_current_time_microsecs() %
                RANDOM_SEQUENCE_SIZE) % 50) * 0.01f;
    
    to_construct->linear_distance_per_second = 0.0f;
    to_construct->linear_direction[0] =  0.1f;
    to_construct->linear_direction[1] =  0.1f;
    to_construct->linear_direction[2] = -0.8f;
    
    to_construct->squared_distance_per_second = 0.0f;
    to_construct->squared_direction[0] = -0.1f;
    to_construct->squared_direction[1] = 1.0f;
    to_construct->squared_direction[2] = 0.2f;
    
    to_construct->rgb_bonus_per_second[0] = 0.0f;
    to_construct->rgb_bonus_per_second[1] = 0.0f;
    to_construct->rgb_bonus_per_second[2] = 0.0f;
    
    to_construct->xyz_rotation_per_second[0] =
        (float)(tok_rand_at_i((5 +
            platform_get_current_time_microsecs()) %
                RANDOM_SEQUENCE_SIZE) % 628) * 0.01f;
    to_construct->xyz_rotation_per_second[1] =
        (float)(tok_rand_at_i((3 +
            platform_get_current_time_microsecs()) %
                RANDOM_SEQUENCE_SIZE) % 628) * 0.01f;
    to_construct->xyz_rotation_per_second[2] =
        (float)(tok_rand_at_i((19 +
            platform_get_current_time_microsecs()) %
                RANDOM_SEQUENCE_SIZE) % 628) * 0.01f;
}

ShatterEffect * next_shatter_effect()
{
    for (uint32_t i = 0; i < shatter_effects_size; i++) {
        if (shatter_effects[i].deleted) {
            construct_shatter_effect_no_zpoly(&shatter_effects[i]);
            return &shatter_effects[i];
        }
    }
    
    shatter_effects_size += 1;
    construct_shatter_effect_no_zpoly(
        &shatter_effects[shatter_effects_size - 1]);
    
    return &shatter_effects[shatter_effects_size - 1];
}

ShatterEffect * next_shatter_effect_with_zpoly(
    zPolygonCPU * construct_with_zpolygon,
    GPUPolygon * construct_with_polygon_gpu,
    GPUPolygonMaterial * construct_with_polygon_material)
{
    ShatterEffect * return_value = next_shatter_effect();
    
    log_assert(shatter_effects_size < SHATTER_EFFECTS_SIZE);
    return_value->zpolygon_to_shatter_cpu =
        *construct_with_zpolygon;
    return_value->zpolygon_to_shatter_gpu =
        *construct_with_polygon_gpu;
    return_value->zpolygon_to_shatter_material =
        *construct_with_polygon_material;
    
    return return_value;
}

void commit_shatter_effect(
    ShatterEffect * to_commit)
{
    // if the zpolygon has too few triangles, it won't make for an interesting
    // particle effect. We will 'shatter' the triangles and point to a split up
    // version. If there's enough triangles, we will simply set the 'shattered'
    // pointers to be the same as the original pointers
    if (
        all_mesh_summaries[to_commit->zpolygon_to_shatter_cpu.mesh_id]
            .shattered_vertices_size == 0)
    {
        if (
            all_mesh_summaries[to_commit->zpolygon_to_shatter_cpu.mesh_id]
                .vertices_size <
                    MINIMUM_SHATTER_TRIANGLES)
        {
             // this has too few triangles, so shatter them into pieces
             create_shattered_version_of_mesh(
                 /* mesh_id: */
                     to_commit->zpolygon_to_shatter_cpu.mesh_id,
                 /* triangles_multiplier: */
                     (uint32_t)(1 + (
                         MINIMUM_SHATTER_TRIANGLES /
                             all_mesh_summaries[
                                 to_commit->zpolygon_to_shatter_cpu.mesh_id].
                                     vertices_size)));
        } else {
            // use the original as the shattered version
            all_mesh_summaries[to_commit->zpolygon_to_shatter_cpu.mesh_id].
                shattered_vertices_size =
                    all_mesh_summaries[
                        to_commit->zpolygon_to_shatter_cpu.mesh_id].
                            vertices_size;
            
            all_mesh_summaries[to_commit->zpolygon_to_shatter_cpu.mesh_id].
                shattered_vertices_head_i =
            all_mesh_summaries[
                to_commit->zpolygon_to_shatter_cpu.mesh_id].vertices_head_i;
        }
    }
    
    normalize_vertex(to_commit->linear_direction);
    normalize_vertex(to_commit->squared_direction);
        
    to_commit->committed = true;
}

void add_shatter_effects_to_workload(
    GPUDataForSingleFrame * frame_data,
    uint64_t elapsed_nanoseconds)
{
    for (uint32_t i = 0; i < shatter_effects_size; i++) {
        if (shatter_effects[i].deleted ||
            !shatter_effects[i].committed ||
            !shatter_effects[i].zpolygon_to_shatter_cpu.committed ||
            shatter_effects[i].zpolygon_to_shatter_cpu.deleted)
        {
            continue;
        }
        
        if (shatter_effects[i].wait_first > elapsed_nanoseconds) {
            shatter_effects[i].wait_first -= elapsed_nanoseconds;
            continue;
        } else if (shatter_effects[i].wait_first > 0) {
            shatter_effects[i].wait_first = 0;
            elapsed_nanoseconds -= shatter_effects[i].wait_first;
        }
        
        shatter_effects[i].elapsed += elapsed_nanoseconds;
        
        int32_t shatter_head_i =
            all_mesh_summaries[
                shatter_effects[i].zpolygon_to_shatter_cpu.mesh_id].
                    shattered_vertices_head_i;
        log_assert(shatter_head_i >= 0);
        
        int32_t shatter_tail_i =
            shatter_head_i +
            all_mesh_summaries[
                shatter_effects[i].zpolygon_to_shatter_cpu.mesh_id].
                    shattered_vertices_size;
        log_assert(shatter_tail_i >= 0);
         
        uint64_t lifetime_so_far = shatter_effects[i].elapsed;
        if (
            lifetime_so_far >
                shatter_effects[i].finish_fade_out_at_elapsed +
                    shatter_effects[i].longest_random_delay_before_launch)
        {
            shatter_effects[i].deleted = true;
            continue;
        }
        
        for (
            int32_t vert_i = shatter_head_i;
            vert_i < (shatter_tail_i - 1);
            vert_i += 3)
        {
            uint64_t random_delay =
                tok_rand_at_i(
                    ((shatter_effects[i].random_seed + (uint32_t)vert_i) %
                        (RANDOM_SEQUENCE_SIZE - 1))
                    ) %
                shatter_effects[i].longest_random_delay_before_launch;
            
            uint64_t delayed_lifetime_so_far =
                lifetime_so_far > random_delay ?
                    lifetime_so_far - random_delay : 0;
            log_assert(delayed_lifetime_so_far < shatter_effects[i].elapsed);
            
            float alpha = 1.0f;
            
            if (
                delayed_lifetime_so_far >
                    shatter_effects[i].finish_fade_out_at_elapsed)
            {
                alpha = 0.0f;
            } else if (
                delayed_lifetime_so_far >
                    shatter_effects[i].start_fade_out_at_elapsed)
            {
                alpha =
                    1.0f -
                    ((float)(delayed_lifetime_so_far -
                        shatter_effects[i].start_fade_out_at_elapsed) /
                    (float)(shatter_effects[i].finish_fade_out_at_elapsed -
                        shatter_effects[i].start_fade_out_at_elapsed));
                log_assert(alpha >= 0.0f);
                log_assert(alpha < 1.0f);
            }
            
            zVertex exploding_direction;
            exploding_direction.x =
                 ((
                    all_mesh_vertices->gpu_data[vert_i + 0].xyz[0] +
                    all_mesh_vertices->gpu_data[vert_i + 1].xyz[0] +
                    all_mesh_vertices->gpu_data[vert_i + 2].xyz[0]) / 3) -
                        shatter_effects[i].zpolygon_to_shatter_gpu.xyz[0];
            exploding_direction.y =
                ((
                    all_mesh_vertices->gpu_data[vert_i + 0].xyz[1] +
                    all_mesh_vertices->gpu_data[vert_i + 1].xyz[1] +
                    all_mesh_vertices->gpu_data[vert_i + 2].xyz[1]) / 3) -
                        shatter_effects[i].zpolygon_to_shatter_gpu.xyz[1];
            exploding_direction.z =
                ((
                    all_mesh_vertices->gpu_data[vert_i + 0].xyz[2] +
                    all_mesh_vertices->gpu_data[vert_i + 1].xyz[2] +
                    all_mesh_vertices->gpu_data[vert_i + 2].xyz[2]) / 3) -
                        shatter_effects[i].zpolygon_to_shatter_gpu.xyz[2];
            normalize_zvertex(&exploding_direction);
            
            float rotation[3];
            for (uint32_t r = 0; r < 3; r++) {
                rotation[r] =
                    ((float)delayed_lifetime_so_far / 1000000.0f) *
                        shatter_effects[i].xyz_rotation_per_second[r];
            }
            float exploding_distance_traveled =
                ((float)delayed_lifetime_so_far / 1000000.0f) *
                    shatter_effects[i].exploding_distance_per_second;
            float linear_distance_traveled =
                ((float)delayed_lifetime_so_far / 1000000.0f) *
                    shatter_effects[i].linear_distance_per_second;
            float sq_distance_traveled =
                (((float)delayed_lifetime_so_far / 1000000.0f) *
                ((float)delayed_lifetime_so_far / 1000000.0f)) *
                    shatter_effects[i].squared_distance_per_second;
            
            frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size] =
                    shatter_effects[i].zpolygon_to_shatter_gpu;
            
            frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size].xyz[0] +=
                    (linear_distance_traveled *
                        shatter_effects[i].linear_direction[0]) +
                    (sq_distance_traveled *
                        shatter_effects[i].squared_direction[0]) +
                    (exploding_distance_traveled *
                        exploding_direction.x);
            
            frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size].xyz[1] +=
                    (linear_distance_traveled *
                        shatter_effects[i].linear_direction[1]) +
                    (sq_distance_traveled *
                        shatter_effects[i].squared_direction[1]) +
                    (exploding_distance_traveled *
                        exploding_direction.y);
            
            frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size].xyz[2] +=
                        (linear_distance_traveled *
                            shatter_effects[i].linear_direction[2]) +
                        (sq_distance_traveled *
                            shatter_effects[i].squared_direction[2]) +
                        (exploding_distance_traveled *
                            exploding_direction.z);
            
            frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size].xyz_angle[0] +=
                    rotation[0];
            frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size].xyz_angle[1] +=
                    rotation[1];
            frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size].xyz_angle[2] +=
                    rotation[2];
            
            frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size].bonus_rgb[0] +=
                    ((float)delayed_lifetime_so_far / 1000000.0f) *
                        shatter_effects[i].rgb_bonus_per_second[0];
            frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size].bonus_rgb[1] +=
                    ((float)delayed_lifetime_so_far / 1000000.0f) *
                        shatter_effects[i].rgb_bonus_per_second[1];
            frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size].bonus_rgb[2] +=
                    ((float)delayed_lifetime_so_far / 1000000.0f) *
                        shatter_effects[i].rgb_bonus_per_second[2];
            
            frame_data->polygon_materials[
                frame_data->polygon_collection->size *
                    MAX_MATERIALS_SIZE] =
                        shatter_effects[i].zpolygon_to_shatter_material;
            
            frame_data->polygon_materials[
                frame_data->polygon_collection->size *
                    MAX_MATERIALS_SIZE].rgba[3] = alpha;
            
            for (uint32_t m = 0; m < 3; m++) {
                frame_data->vertices[frame_data->vertices_size].
                    locked_vertex_i = (vert_i + (int32_t)m);
                frame_data->vertices[frame_data->vertices_size].polygon_i =
                    (int)frame_data->polygon_collection->size;
                
                if (frame_data->vertices_size + 1 >= MAX_VERTICES_PER_BUFFER) {
                    return;
                }
                frame_data->vertices_size += 1;
            }
            
            if (frame_data->polygon_collection->size + 1 <
                MAX_POLYGONS_PER_BUFFER)
            {
                frame_data->polygon_collection->size += 1;
            } else {
                return;
            }
        }
    }
}

ParticleEffect * particle_effects;
uint32_t particle_effects_size;

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
    to_construct->max_color_variance = 0.05f;
    
    to_construct->random_textures_size = 0;
    
    to_construct->particle_direction[0] = 1.0f;
    to_construct->particle_direction[1] = 0.0f;
    to_construct->particle_direction[2] = 0.0f;
    to_construct->particle_distance_per_second = 0.5f;
    to_construct->particle_distance_max_variance = 0;
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
    
    log_assert(
        to_request->mesh_id_to_spawn >= 0);
    log_assert(
        (uint32_t)to_request->mesh_id_to_spawn < all_mesh_summaries_size);
    
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
    
    assert(to_request->random_textures_size < MAX_PARTICLE_TEXTURES);
    for (
        uint32_t i = 0;
        (int32_t)i < to_request->random_textures_size;
        i++)
    {
        assert(
            to_request->random_texturearray_i[i] <
                TEXTUREARRAYS_SIZE);
        
        assert(
            to_request->random_texture_i[i] <
                MAX_FILES_IN_SINGLE_TEXARRAY);
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

static void adjust_colors_by_random(
    float * out_red,
    float * out_green,
    float * out_blue,
    const float max_variance,
    const uint64_t at_random_seed)
{
    float red_bonus =
        (max_variance / 100.0f) * (tok_rand_at_i(at_random_seed + 0) % 100) -
        (max_variance / 100.0f) * (tok_rand_at_i(at_random_seed + 11) % 100);
    float green_bonus =
        (max_variance / 100.0f) * (tok_rand_at_i(at_random_seed + 12) % 100) -
        (max_variance / 100.0f) * (tok_rand_at_i(at_random_seed + 3) % 100);
    float blue_bonus =
        (max_variance / 100.0f) * (tok_rand_at_i(at_random_seed + 4) % 100) -
        (max_variance / 100.0f) * (tok_rand_at_i(at_random_seed + 15) % 100);
    
    *out_red += red_bonus;
    if (*out_red < 0.0f) { *out_red = 0.0f; }
    if (*out_red > 1.0f) { *out_red = 1.0f; }
    
    *out_green += green_bonus;
    if (*out_green < 0.0f) { *out_green = 0.0f; }
    if (*out_green > 1.0f) { *out_green = 1.0f; }
    
    *out_blue += blue_bonus;
    if (*out_blue < 0.0f) { *out_blue = 0.0f; }
    if (*out_blue > 1.0f) { *out_blue = 1.0f; }
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
    if (transition_progress > 1.0f) {
        transition_progress = 1.0f;
    }
    
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
    GPUDataForSingleFrame * frame_data,
    uint64_t elapsed_nanoseconds)
{
    zVertex randomized_direction;
    zVertex randomized_squared_direction;
    
    uint64_t spawns_in_duration;
    uint64_t interval_between_spawns;
    uint64_t spawn_lifetime_so_far;
    
    for (
        uint32_t i = 0;
        i < particle_effects_size;
        i++)
    {
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
                    (particle_effects[i].random_seed
                        + (spawn_i * 41) + (spawn_i % 3)) %
                        (RANDOM_SEQUENCE_SIZE - 50);
                
                int32_t texturearray_i = -1;
                int32_t texture_i = -1;
                
                if (particle_effects[i].random_textures_size > 0) {
                    int32_t rand_texture_i =
                        (int32_t)tok_rand_at_i(rand_i + 12) %
                            particle_effects[i].random_textures_size;
                    
                    texturearray_i = particle_effects[i].
                        random_texturearray_i[rand_texture_i];
                    assert(texturearray_i < TEXTUREARRAYS_SIZE);
                    texture_i = particle_effects[i].
                        random_texture_i[rand_texture_i];
                    assert(texture_i < MAX_FILES_IN_SINGLE_TEXARRAY);
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
                
                // distance variance
                float dist_pos = 0;
                float dist_neg = 0;
                if (particle_effects[i].particle_distance_max_variance > 0) {
                    dist_pos = (float)(
                            tok_rand_at_i(rand_i + 19) %
                                particle_effects[i].
                                    particle_distance_max_variance) /
                                        100.0f;
                    dist_neg = (float)(
                        tok_rand_at_i(rand_i + 20) %
                            particle_effects[i].
                                particle_distance_max_variance) /
                                    100.0f;
                }
                
                float distance_traveled =
                    ((float)spawn_lifetime_so_far / 1000000.0f) *
                        (
                            particle_effects[i].particle_distance_per_second +
                            dist_pos +
                            dist_neg);
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
                        tok_rand_at_i(rand_i + 17) %
                            particle_effects[i].
                                squared_direction_max_z_angle_variance) /
                                    100.0f;
                    float z_rotation_neg = (float)(
                        tok_rand_at_i(rand_i + 18) %
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
                
                adjust_colors_by_random(
                    &red,
                    &green,
                    &blue,
                    /* max_variance: */ particle_effects[i].max_color_variance,
                    /* random_seed: */ rand_i);
                
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
                                particle_origin_max_y_variance) /
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
                    int32_t vert_i = all_mesh_summaries[
                        particle_effects[i].mesh_id_to_spawn].vertices_head_i;
                    vert_i <
                        all_mesh_summaries[
                            particle_effects[i].mesh_id_to_spawn].
                                vertices_head_i +
                        all_mesh_summaries[
                            particle_effects[i].mesh_id_to_spawn].
                                vertices_size;
                    vert_i++)
                {
                    log_assert(vert_i >= 0);
                    log_assert(vert_i < (int32_t)all_mesh_vertices->size);
                    
                    frame_data->vertices[frame_data->vertices_size].
                        locked_vertex_i = vert_i;
                    frame_data->vertices[frame_data->vertices_size].polygon_i =
                        (int)frame_data->polygon_collection->size;
                    
                    frame_data->vertices_size += 1;
                    log_assert(
                        frame_data->vertices_size < MAX_VERTICES_PER_BUFFER);
                }
                
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].xyz[0] =
                    (particle_effects[i].x + initial_x_offset) +
                        (distance_traveled * randomized_direction.x) +
                        (sq_distance_traveled *
                            randomized_squared_direction.x);
                
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].xyz[1] =
                    (particle_effects[i].y + initial_y_offset) +
                        (distance_traveled * randomized_direction.y) +
                        (sq_distance_traveled *
                            randomized_squared_direction.y);
                
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].xyz[2] =
                    (particle_effects[i].z + initial_z_offset) +
                        (distance_traveled * randomized_direction.z) +
                        (sq_distance_traveled *
                            randomized_squared_direction.z);
                
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].xyz_multiplier[0] =
                        particle_effects[i].particle_x_multiplier;
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].xyz_multiplier[1] =
                        particle_effects[i].particle_y_multiplier;
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].xyz_multiplier[2] =
                        particle_effects[i].particle_z_multiplier;

                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].xyz_offset[0] = 0.0f;
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].xyz_offset[1] = 0.0f;
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].xyz_offset[2] = 0.0f;
                
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].ignore_lighting =
                        particle_effects[i].particles_ignore_lighting;
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].ignore_camera =
                        false;
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].scale_factor = 1.0f;
                
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].xyz_angle[0] = 0.0f;
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].xyz_angle[1] = 0.0f;
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].xyz_angle[2] = 0.0f;
                
                frame_data->polygon_materials[
                    frame_data->polygon_collection->size * MAX_MATERIALS_SIZE].
                        rgba[0] =
                            red;
                frame_data->polygon_materials[
                    frame_data->polygon_collection->size * MAX_MATERIALS_SIZE].
                        rgba[1] =
                            green;
                frame_data->polygon_materials[
                    frame_data->polygon_collection->size * MAX_MATERIALS_SIZE].
                        rgba[2] =
                            blue;
                frame_data->polygon_materials[
                    frame_data->polygon_collection->size * MAX_MATERIALS_SIZE].
                        rgba[3] =
                            alpha;
                
                frame_data->polygon_materials[
                    frame_data->polygon_collection->size * MAX_MATERIALS_SIZE].
                        texturearray_i = texturearray_i;
                frame_data->polygon_materials[
                    frame_data->polygon_collection->size * MAX_MATERIALS_SIZE].
                        texture_i = texture_i;
                
                frame_data->polygon_collection->size += 1;
            }
            frame_data->first_line_i = frame_data->vertices_size;
            
            if (particles_active < 1) {
                particle_effects[i].random_seed =
                    tok_rand() % RANDOM_SEQUENCE_SIZE;
            } else if (particle_effects[i].generate_light) {
                frame_data->light_collection->light_x[
                    frame_data->light_collection->lights_size] =
                        particle_effects[i].x;
                frame_data->light_collection->light_y[
                    frame_data->light_collection->lights_size] =
                        particle_effects[i].y;
                frame_data->light_collection->light_z[
                    frame_data->light_collection->lights_size] =
                        particle_effects[i].z;
                
                frame_data->light_collection->red[
                    frame_data->light_collection->lights_size] =
                        particle_effects[i].light_rgb[0];
                frame_data->light_collection->green[
                    frame_data->light_collection->lights_size] =
                        particle_effects[i].light_rgb[1];
                frame_data->light_collection->blue[
                    frame_data->light_collection->lights_size] =
                        particle_effects[i].light_rgb[2];
                
                frame_data->light_collection->reach[
                    frame_data->light_collection->lights_size] =
                        particle_effects[i].light_reach;
                
                float light_strength =
                    particle_effects[i].light_strength * (
                        (float)particles_active / (float)spawns_in_duration);
                
                frame_data->light_collection->ambient[
                    frame_data->light_collection->lights_size] =
                            0.05f * light_strength;
                frame_data->light_collection->diffuse[
                    frame_data->light_collection->lights_size] =
                        1.0f * light_strength;
                
                frame_data->light_collection->lights_size += 1;
            }
        }
    }
}
