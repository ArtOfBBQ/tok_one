#include "particle.h"


LineParticle * lineparticle_effects;
uint32_t lineparticle_effects_size;

static void construct_lineparticle_effect_no_zpoly(
    LineParticle * to_construct)
{
    to_construct->random_seed =
        (uint64_t)tok_rand_at_i(
            platform_get_current_time_microsecs() %
                RANDOM_SEQUENCE_SIZE) % 75;
    to_construct->elapsed = 0;
    to_construct->wait_first = 0;
    to_construct->committed = false;
    to_construct->deleted = false;
    
    to_construct->waypoints_size = 1;
    to_construct->waypoint_x[0] = 0.2f;
    to_construct->waypoint_y[0] = 0.2f;
    to_construct->waypoint_z[0] = 0.0f;
    to_construct->waypoint_r[0] = 1.0f;
    to_construct->waypoint_g[0] = 1.0f;
    to_construct->waypoint_b[0] = 0.8f;
    to_construct->waypoint_a[0] = 1.0f;
    to_construct->waypoint_scalefactor[0] = 0.1f;
    
    to_construct->waypoint_x[1] = 1.0f;
    to_construct->waypoint_y[1] = 1.0f;
    to_construct->waypoint_z[1] = 0.0f;
    to_construct->waypoint_r[1] = 1.0f;
    to_construct->waypoint_g[1] = 0.8f;
    to_construct->waypoint_b[1] = 1.0f;
    to_construct->waypoint_a[1] = 1.0f;
    to_construct->waypoint_scalefactor[1] = 1.0f;
    
    to_construct->waypoints_size = 2;
    to_construct->waypoint_duration[0] = 500000;
    to_construct->trail_delay = 300000;
    to_construct->particle_count = 500;
    to_construct->particle_zangle_variance_pct = 314;
    to_construct->particle_rgb_variance_pct = 15;
    to_construct->particle_scalefactor_variance_pct = 25;
}

LineParticle * next_lineparticle_effect(void)
{
    for (uint32_t i = 0; i < lineparticle_effects_size; i++) {
        if (lineparticle_effects[i].deleted) {
            construct_lineparticle_effect_no_zpoly(&lineparticle_effects[i]);
            return &lineparticle_effects[i];
        }
    }
    
    lineparticle_effects_size += 1;
    log_assert(lineparticle_effects_size < LINEPARTICLE_EFFECTS_SIZE);
    
    construct_lineparticle_effect_no_zpoly(
        &lineparticle_effects[lineparticle_effects_size - 1]);
    
    return &lineparticle_effects[lineparticle_effects_size - 1];
}

LineParticle * next_lineparticle_effect_with_zpoly(
    zPolygonCPU * construct_with_zpolygon,
    GPUPolygon * construct_with_polygon_gpu,
    GPUPolygonMaterial * construct_with_polygon_material)
{
    LineParticle * return_value = next_lineparticle_effect();
    
    log_assert(lineparticle_effects_size < LINEPARTICLE_EFFECTS_SIZE);
    return_value->zpolygon_cpu = *construct_with_zpolygon;
    return_value->zpolygon_gpu = *construct_with_polygon_gpu;
    return_value->zpolygon_material = *construct_with_polygon_material;
    
    return return_value;
}

void commit_lineparticle_effect(
    LineParticle * to_commit)
{
    log_assert(!to_commit->deleted);
    log_assert(to_commit->waypoints_size > 1);
    log_assert(to_commit->zpolygon_cpu.committed);
    log_assert(!to_commit->zpolygon_cpu.deleted);
    log_assert(to_commit->zpolygon_material.rgba[0] < 1.05f);
    log_assert(to_commit->zpolygon_material.rgba[1] < 1.05f);
    log_assert(to_commit->zpolygon_material.rgba[2] < 1.05f);
    log_assert(to_commit->zpolygon_material.rgba[3] < 1.05f);
    log_assert(to_commit->zpolygon_material.rgba[0] > -0.01f);
    log_assert(to_commit->zpolygon_material.rgba[1] > -0.01f);
    log_assert(to_commit->zpolygon_material.rgba[2] > -0.01f);
    log_assert(to_commit->zpolygon_material.rgba[3] > -0.01f);
    log_assert(to_commit->zpolygon_material.diffuse  > -0.01f);
    log_assert(to_commit->zpolygon_material.specular > -0.01f);
    log_assert(to_commit->zpolygon_material.diffuse  <  1.01f);
    log_assert(to_commit->zpolygon_material.specular <  1.01f);
    log_assert(to_commit->zpolygon_gpu.xyz_multiplier[0] > 0.0f);
    log_assert(to_commit->zpolygon_gpu.xyz_multiplier[1] > 0.0f);
    log_assert(to_commit->zpolygon_gpu.xyz_multiplier[2] > 0.0f);
    
    to_commit->committed = true;
    to_commit->random_seed = (uint32_t)tok_rand() % RANDOM_SEQUENCE_SIZE;
}

#define add_variance(x, variance, randnum, randnum2) if (variance > 0) { x += ((float)(randnum % variance) * 0.01f); x -= ((float)(randnum2 % variance) * 0.01f); }

void add_lineparticle_effects_to_workload(
    GPUDataForSingleFrame * frame_data,
    uint64_t elapsed_nanoseconds,
    const bool32_t alpha_blending)
{
    for (uint32_t i = 0; i < lineparticle_effects_size; i++) {
        if (
            lineparticle_effects[i].deleted ||
            !lineparticle_effects[i].committed ||
            !lineparticle_effects[i].zpolygon_cpu.committed ||
            lineparticle_effects[i].zpolygon_cpu.deleted ||
            lineparticle_effects[i].zpolygon_cpu.alpha_blending_enabled !=
                alpha_blending)
        {
            continue;
        }
        
        if (lineparticle_effects[i].wait_first > elapsed_nanoseconds) {
            lineparticle_effects[i].wait_first -= elapsed_nanoseconds;
            continue;
        } else if (lineparticle_effects[i].wait_first > 0) {
            lineparticle_effects[i].wait_first = 0;
            elapsed_nanoseconds -= lineparticle_effects[i].wait_first;
        }
        
        lineparticle_effects[i].elapsed += elapsed_nanoseconds;
        
        int32_t head_i =
            all_mesh_summaries[
                lineparticle_effects[i].zpolygon_cpu.mesh_id].vertices_head_i;
        log_assert(head_i >= 0);
        
        int32_t tail_i = head_i +
            all_mesh_summaries[
                lineparticle_effects[i].zpolygon_cpu.mesh_id].vertices_size;
        log_assert(tail_i >= 0);
        
        uint64_t lifetime_so_far = lineparticle_effects[i].elapsed;
        uint64_t total_lifetime = 0;
        for (
            uint32_t _ = 0;
            _ < lineparticle_effects[i].waypoints_size;
            _++)
        {
            total_lifetime += lineparticle_effects[i].waypoint_duration[_];
        }
        
        if (
            lifetime_so_far >
                total_lifetime + lineparticle_effects[i].trail_delay)
        {
            lineparticle_effects[i].deleted = true;
            continue;
        }
        
        uint64_t particle_rands[5];
        for (
            uint32_t particle_i = 0;
            particle_i < lineparticle_effects[i].particle_count;
            particle_i++)
        {
            uint64_t particle_delay =
                (lineparticle_effects[i].trail_delay /
                    lineparticle_effects[i].particle_count) *
                        particle_i;
            
            if (particle_delay > lineparticle_effects[i].elapsed) {
                continue;
            }
            
            particle_rands[0] = (uint64_t)tok_rand_at_i(
                (lineparticle_effects[i].random_seed + particle_i) %
                    RANDOM_SEQUENCE_SIZE);
            particle_rands[1] = (uint64_t)tok_rand_at_i(
                (lineparticle_effects[i].random_seed + particle_i + 37) %
                    RANDOM_SEQUENCE_SIZE);
            particle_rands[2] = (uint64_t)tok_rand_at_i(
                (lineparticle_effects[i].random_seed + particle_i + 51) %
                    RANDOM_SEQUENCE_SIZE);
            particle_rands[3] = (uint64_t)tok_rand_at_i(
                (lineparticle_effects[i].random_seed + particle_i + 237) %
                    RANDOM_SEQUENCE_SIZE);
            particle_rands[4] = (uint64_t)tok_rand_at_i(
                (lineparticle_effects[i].random_seed + particle_i + 414) %
                    RANDOM_SEQUENCE_SIZE);
            
            uint64_t delayed_lifetime_so_far =
                    lifetime_so_far > particle_delay ?
                        lifetime_so_far - particle_delay : 0;
            log_assert(
                delayed_lifetime_so_far <= lineparticle_effects[i].elapsed);
            
            uint64_t elapsed_in_this_waypoint = delayed_lifetime_so_far;
            uint32_t prev_i = 0;
            uint32_t next_i = 1;
            
            while (
                prev_i < MAX_LINEPARTICLE_DIRECTIONS &&
                elapsed_in_this_waypoint > lineparticle_effects[i].
                    waypoint_duration[prev_i])
            {
                elapsed_in_this_waypoint -= lineparticle_effects[i].
                    waypoint_duration[prev_i];
                prev_i += 1;
                next_i += 1;
            }
            
            if (
                next_i >= lineparticle_effects[i].waypoints_size ||
                prev_i >= MAX_LINEPARTICLE_DIRECTIONS)
            {
                continue;
            }
            
            frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size] =
                    lineparticle_effects[i].zpolygon_gpu;
            
            float next_multiplier =
                (float)elapsed_in_this_waypoint /
                    (float)lineparticle_effects[i].
                        waypoint_duration[prev_i];
            log_assert(next_multiplier < 1.01f);
            log_assert(next_multiplier > -0.01f);
            float prev_multiplier = 1.0f - next_multiplier;
            
            frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size].xyz[0] =
                    (prev_multiplier * lineparticle_effects[i].
                        waypoint_x[prev_i]) +
                    (next_multiplier * lineparticle_effects[i].
                        waypoint_x[next_i]);
            frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size].xyz[1] =
                    (prev_multiplier * lineparticle_effects[i].
                        waypoint_y[prev_i]) +
                    (next_multiplier * lineparticle_effects[i].
                        waypoint_y[next_i]);
            frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size].xyz[2] =
                    (prev_multiplier * lineparticle_effects[i].
                        waypoint_z[prev_i]) +
                    (next_multiplier * lineparticle_effects[i].
                        waypoint_z[next_i]);
            
            frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size].scale_factor =
                    (prev_multiplier * lineparticle_effects[i].
                        waypoint_scalefactor[prev_i]) +
                    (next_multiplier * lineparticle_effects[i].
                        waypoint_scalefactor[next_i]);
            add_variance(
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].scale_factor,
                lineparticle_effects[i].
                    particle_scalefactor_variance_pct,
                particle_rands[0],
                particle_rands[1]);
            
            log_assert(
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].xyz_offset[0] == 0);
            log_assert(
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].xyz_offset[0] == 0);
            log_assert(
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].xyz_offset[0] == 0);
            log_assert(
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].bonus_rgb[0] == 0);
            log_assert(
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].bonus_rgb[1] == 0);
            log_assert(
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].bonus_rgb[2] == 0);
            
            frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size].xyz_multiplier[0] =
                    lineparticle_effects[i].zpolygon_gpu.xyz_multiplier[0];
            frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size].xyz_multiplier[1] =
                    lineparticle_effects[i].zpolygon_gpu.xyz_multiplier[1];
            frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size].xyz_multiplier[2] =
                    lineparticle_effects[i].zpolygon_gpu.xyz_multiplier[2];
            
            frame_data->polygon_materials[
                frame_data->polygon_collection->size *
                    MAX_MATERIALS_PER_POLYGON] =
                        lineparticle_effects[i].zpolygon_material;
            
            frame_data->polygon_materials[
                frame_data->polygon_collection->size *
                    MAX_MATERIALS_PER_POLYGON].rgba[0] =
                        (lineparticle_effects[i].waypoint_r[prev_i] *
                            prev_multiplier) +
                        (lineparticle_effects[i].waypoint_r[next_i] *
                            next_multiplier);
            add_variance(
                frame_data->polygon_materials[
                    frame_data->polygon_collection->size *
                        MAX_MATERIALS_PER_POLYGON].rgba[0],
                lineparticle_effects[i].particle_rgb_variance_pct,
                particle_rands[2],
                particle_rands[3]);
            
            frame_data->polygon_materials[
                frame_data->polygon_collection->size *
                    MAX_MATERIALS_PER_POLYGON].rgba[1] =
                        (lineparticle_effects[i].waypoint_g[prev_i] *
                            prev_multiplier) +
                        (lineparticle_effects[i].waypoint_g[next_i] *
                            next_multiplier);
            add_variance(
                frame_data->polygon_materials[
                    frame_data->polygon_collection->size *
                        MAX_MATERIALS_PER_POLYGON].rgba[1],
                lineparticle_effects[i].particle_rgb_variance_pct,
                particle_rands[3],
                particle_rands[4]);
            
            frame_data->polygon_materials[
                frame_data->polygon_collection->size *
                    MAX_MATERIALS_PER_POLYGON].rgba[2] =
                        (lineparticle_effects[i].waypoint_b[prev_i] *
                            prev_multiplier) +
                        (lineparticle_effects[i].waypoint_b[next_i] *
                            next_multiplier);
            add_variance(
                frame_data->polygon_materials[
                    frame_data->polygon_collection->size *
                        MAX_MATERIALS_PER_POLYGON].rgba[2],
                lineparticle_effects[i].particle_rgb_variance_pct,
                particle_rands[1],
                particle_rands[3]);
            
            frame_data->polygon_materials[
                frame_data->polygon_collection->size *
                    MAX_MATERIALS_PER_POLYGON].rgba[3] =
                        ((lineparticle_effects[i].waypoint_a[prev_i] *
                            prev_multiplier) +
                        (lineparticle_effects[i].waypoint_a[next_i] *
                            next_multiplier));
            add_variance(
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].xyz_angle[2],
                lineparticle_effects[i].particle_zangle_variance_pct,
                particle_rands[2],
                particle_rands[4]);
            
            frame_data->polygon_materials[
                frame_data->polygon_collection->size *
                    MAX_MATERIALS_PER_POLYGON].diffuse = 1.5f;
            frame_data->polygon_materials[
                frame_data->polygon_collection->size *
                    MAX_MATERIALS_PER_POLYGON].specular = 0.75f;
            
            for (
                int32_t vert_i = head_i;
                vert_i < (tail_i - 1);
                vert_i += 3)
            {
                for (uint32_t m = 0; m < 3; m++) {
                    frame_data->vertices[frame_data->vertices_size].
                        locked_vertex_i = (vert_i + (int32_t)m);
                    frame_data->vertices[frame_data->vertices_size].polygon_i =
                        (int)frame_data->polygon_collection->size;
                    
                    if (
                        frame_data->vertices_size + 1 >=
                            MAX_VERTICES_PER_BUFFER)
                    {
                        return;
                    }
                    frame_data->vertices_size += 1;
                }
            }
                
            if (
                frame_data->polygon_collection->size + 1 <
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
    memset(to_construct, 0, sizeof(ParticleEffect));
    
    PolygonRequest poly_request;
    poly_request.cpu_data       = &to_construct->zpolygon_cpu;
    poly_request.gpu_data       = &to_construct->zpolygon_gpu;
    poly_request.gpu_materials  = to_construct->zpolygon_materials;
    poly_request.materials_size = MAX_MATERIALS_PER_POLYGON;
    construct_zpolygon(/* PolygonRequest *to_construct: */ &poly_request);
    
    to_construct->object_id            = -1;
    to_construct->zpolygon_cpu.mesh_id =  1;
    
    to_construct->zpolygon_cpu.committed = true;
    
    to_construct->zpolygon_gpu.scale_factor      = 1.0f;
    to_construct->zpolygon_gpu.xyz_multiplier[0] = 0.01f;
    to_construct->zpolygon_gpu.xyz_multiplier[1] = 0.01f;
    to_construct->zpolygon_gpu.xyz_multiplier[2] = 0.01f;
    to_construct->zpolygon_gpu.ignore_lighting   = true;
    
    to_construct->random_seed = (uint32_t)
        tok_rand() % (RANDOM_SEQUENCE_SIZE - 100);
    to_construct->particle_spawns_per_second = 200;
    to_construct->vertices_per_particle      = 6;
    to_construct->particle_lifespan          = 2000000;
}

ParticleEffect * next_particle_effect(void) {
    ParticleEffect * return_value = NULL;
    
    for (uint32_t i = 0; i < particle_effects_size; i++) {
        if (particle_effects[i].deleted) {
            return_value = &particle_effects[i];
            break;
        }
    }
    
    if (return_value == NULL) {
        log_assert(particle_effects_size + 1 < PARTICLE_EFFECTS_SIZE);
        return_value = &particle_effects[particle_effects_size];
        particle_effects_size += 1;
    }
    
    construct_particle_effect(return_value);
    return return_value;
}

void commit_particle_effect(
    ParticleEffect * to_request)
{
    log_assert(
        to_request->zpolygon_cpu.mesh_id >= 0);
    log_assert(
        to_request->zpolygon_cpu.visible);
    log_assert(
        to_request->zpolygon_materials[0].rgba[3] > 0.05f);
    
    // Reminder: The particle effect is not committed, but the zpoly should be
    log_assert(to_request->zpolygon_cpu.committed);
    
    log_assert(
        (uint32_t)to_request->zpolygon_cpu.mesh_id < all_mesh_summaries_size);
    log_assert(
        !to_request->deleted);
    
    // Reminder: The particle effect is not committed, but the zpoly should be
    log_assert(!to_request->committed);
    
    log_assert(to_request->particle_lifespan > 0);
    log_assert(to_request->elapsed == 0);
    log_assert(to_request->particle_spawns_per_second > 0);
    log_assert(to_request->vertices_per_particle > 0);
    log_assert(to_request->vertices_per_particle % 3 == 0);
    
    to_request->committed = true;
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
    GPUDataForSingleFrame * frame_data,
    uint64_t elapsed_nanoseconds,
    const bool32_t alpha_blending)
{
    uint64_t spawns_in_duration;
    uint64_t interval_between_spawns;
    uint64_t spawn_lifetime_so_far;
    
    for (
        uint32_t i = 0;
        i < particle_effects_size;
        i++)
    {
        if (particle_effects[i].deleted ||
            !particle_effects[i].committed ||
            particle_effects[i].zpolygon_cpu.alpha_blending_enabled !=
                alpha_blending)
        {
            continue;
        }
        
        particle_effects[i].elapsed += elapsed_nanoseconds;
        
        if (particle_effects[i].elapsed >
            (particle_effects[i].particle_lifespan +
                particle_effects[i].pause_between_spawns))
        {
            if (particle_effects[i].loops == 1) {
                particle_effects[i].deleted = true;
                continue;
            }
            
            if (particle_effects[i].loops > 1) {
                particle_effects[i].loops -= 1;
            }
            
            particle_effects[i].elapsed = 0;
        }
        
        spawns_in_duration =
            (particle_effects[i].particle_lifespan *
                particle_effects[i].particle_spawns_per_second) /
                    1000000;
        interval_between_spawns =
            particle_effects[i].pause_between_spawns;
        
        uint32_t particles_active = 0;
        
        int32_t vert_head_i = particle_effects[i].use_shattered_mesh ?
            all_mesh_summaries[
                particle_effects[i].zpolygon_cpu.mesh_id].
                    shattered_vertices_head_i :
            all_mesh_summaries[
                particle_effects[i].zpolygon_cpu.mesh_id].
                    vertices_head_i;
        int32_t verts_size = particle_effects[i].use_shattered_mesh ?
            all_mesh_summaries[
                particle_effects[i].zpolygon_cpu.mesh_id].
                    shattered_vertices_size :
            all_mesh_summaries[
                particle_effects[i].zpolygon_cpu.mesh_id].
                    vertices_size;
        int32_t queue_vert_i = 0;
        
        for (
            uint32_t spawn_i = 0;
            spawn_i < spawns_in_duration;
            spawn_i++)
        {
            uint64_t rand_i =
                (particle_effects[i].random_seed
                    + (spawn_i * 9)) %
                    (FLOAT_SEQUENCE_SIZE - 256);
            
            if (particle_effects[i].elapsed <
                (spawn_i * interval_between_spawns))
            {
                continue;
            }
            
            spawn_lifetime_so_far =
                (particle_effects[i].elapsed -
                (spawn_i * interval_between_spawns));
            
            if (spawn_lifetime_so_far > particle_effects[i].particle_lifespan)
            {
                continue;
            }
            
            particles_active += 1;
            
            for (
                int32_t _ = 0;
                _ < (int32_t)particle_effects[i].vertices_per_particle;
                _++)
            {
                int32_t next_vert_i = vert_head_i +
                    (queue_vert_i % verts_size);
                queue_vert_i += 1;
                
                log_assert(next_vert_i >= 0);
                log_assert(next_vert_i < (int32_t)all_mesh_vertices->size);
                
                frame_data->vertices[frame_data->vertices_size].
                    locked_vertex_i = next_vert_i;
                frame_data->vertices[frame_data->vertices_size].polygon_i =
                    (int)frame_data->polygon_collection->size;
                
                frame_data->vertices_size += 1;
                log_assert(
                    frame_data->vertices_size < MAX_VERTICES_PER_BUFFER);
            }
            
            memcpy(
                /* void * dst: */
                    frame_data->polygon_collection->polygons +
                        frame_data->polygon_collection->size,
                /* const void * src: */
                    &particle_effects[i].zpolygon_gpu,
                /* size_t n: */
                    sizeof(GPUPolygon));
            
            memcpy(
                &frame_data->polygon_materials[
                    frame_data->polygon_collection->size *
                        MAX_MATERIALS_PER_POLYGON],
                particle_effects[i].zpolygon_materials,
                sizeof(GPUPolygonMaterial) * MAX_MATERIALS_PER_POLYGON);
            
            if (particle_effects[i].random_textures_size > 0) {
                frame_data->polygon_materials[
                    frame_data->polygon_collection->size *
                        MAX_MATERIALS_PER_POLYGON].texturearray_i =
                            particle_effects[i].random_texturearray_i[
                                spawn_i % particle_effects[i].
                                    random_textures_size];
                
                frame_data->polygon_materials[
                    frame_data->polygon_collection->size *
                        MAX_MATERIALS_PER_POLYGON].texture_i =
                            particle_effects[i].random_texture_i[spawn_i %
                                particle_effects[i].random_textures_size];
            }
            
            float * initial_random_add_1_at = (float *)&particle_effects[i].
                gpustats_initial_random_add_1;
            float * initial_random_add_2_at = (float *)&particle_effects[i].
                gpustats_initial_random_add_2;
            
            float * pertime_add_at = (float *)&particle_effects[i].
                gpustats_pertime_add;
            float * perexptime_add_at = (float *)&particle_effects[i].
                gpustats_perexptime_add;
            float * pertime_random_add_1_at = (float *)&particle_effects[i].
                gpustats_pertime_random_add_1;
            float * pertime_random_add_2_at = (float *)&particle_effects[i].
                gpustats_pertime_random_add_2;
            float * recipient_at = (float *)&frame_data->polygon_collection->
                polygons[frame_data->polygon_collection->size];
            
            float one_million = 1000000.0f;
            float exponential_divisor = 1000.0f;
            float one = 1.0f;
            SIMD_FLOAT simdf_one_million = simd_set_float(one_million);
            float fspawn_lifetime_so_far = (float)spawn_lifetime_so_far;
            SIMD_FLOAT simdf_lifetime = simd_set_float(fspawn_lifetime_so_far);
            SIMD_FLOAT simdf_lifetime_exp = simd_div_floats(
                simdf_lifetime, simd_set_float(exponential_divisor));
            simdf_lifetime_exp = simd_max_floats(
                simdf_lifetime_exp,
                simd_set_float(one));
            simdf_lifetime_exp = simd_mul_floats(
                simdf_lifetime_exp, simdf_lifetime_exp);
            
            for (
                uint32_t j = 0;
                j < (sizeof(GPUPolygon) / sizeof(float));
                j += SIMD_FLOAT_LANES)
            {
                // We expect padding to prevent out of bounds ops
                log_assert((j + SIMD_FLOAT_LANES) * sizeof(float) <=
                    sizeof(GPUPolygon));
                
                SIMD_FLOAT simdf_pertime_add = simd_load_floats(
                    (pertime_add_at + j));
                
                // Add the '1st random over time' data
                SIMD_FLOAT simdf_rand = tok_rand_simd_at_i(
                    (rand_i + ((j/SIMD_FLOAT_LANES) *
                        (SIMD_FLOAT_LANES * 4)))
                            + 0);
                SIMD_FLOAT simdf_pertime_random_add = simd_load_floats(
                    pertime_random_add_1_at + j);
                simdf_pertime_random_add = simd_mul_floats(
                    simdf_pertime_random_add,
                    simdf_rand);
                
                simdf_pertime_add = simd_add_floats(
                    simdf_pertime_add, simdf_pertime_random_add);
                
                // Add the '2nd random over time' data
                simdf_rand = tok_rand_simd_at_i(
                    (rand_i + ((j/SIMD_FLOAT_LANES) *
                        (SIMD_FLOAT_LANES * 4)))
                            + 32);
                simdf_pertime_random_add = simd_load_floats(
                    pertime_random_add_2_at + j);
                simdf_pertime_random_add = simd_mul_floats(
                    simdf_pertime_random_add,
                    simdf_rand);
                simdf_pertime_add = simd_add_floats(
                    simdf_pertime_add, simdf_pertime_random_add);
                
                // Convert per second values to per microsecond effect
                simdf_pertime_add = simd_mul_floats(
                    simdf_pertime_add, simdf_lifetime);
                simdf_pertime_add = simd_div_floats(
                    simdf_pertime_add, simdf_one_million);
                
                SIMD_FLOAT exp_add = simd_load_floats((perexptime_add_at + j));
                exp_add = simd_mul_floats(exp_add, simdf_lifetime_exp);
                exp_add = simd_div_floats(exp_add, simdf_one_million);
                
                SIMD_FLOAT recip = simd_load_floats(recipient_at + j);
                recip = simd_add_floats(recip, simdf_pertime_add);
                recip = simd_add_floats(recip, exp_add);
                
                simdf_rand = tok_rand_simd_at_i(
                    (rand_i + ((j/SIMD_FLOAT_LANES) *
                        (SIMD_FLOAT_LANES * 4)))
                            + 64);
                SIMD_FLOAT simdf_initial_add = simd_load_floats(
                    initial_random_add_1_at + j);
                simdf_initial_add = simd_mul_floats(
                    simdf_initial_add, simdf_rand);
                recip = simd_add_floats(recip, simdf_initial_add);
                
                simdf_rand = tok_rand_simd_at_i(
                    (rand_i + ((j/SIMD_FLOAT_LANES) *
                        (SIMD_FLOAT_LANES * 4)))
                            + 96);
                simdf_initial_add = simd_load_floats(
                    initial_random_add_2_at + j);
                simdf_initial_add = simd_mul_floats(
                    simdf_initial_add, simdf_rand);
                recip = simd_add_floats(recip, simdf_initial_add);
                
                log_assert((ptrdiff_t)(recipient_at + j) %
                    (long)(SIMD_FLOAT_LANES * sizeof(float)) == 0);
                simd_store_floats((recipient_at + j), recip);
            }
            if (frame_data->polygon_collection->polygons[
                frame_data->polygon_collection->size].scale_factor < 0.01f)
            {
                frame_data->polygon_collection->polygons[
                    frame_data->polygon_collection->size].scale_factor = 0.01f;
            }
            
            frame_data->polygon_collection->size += 1;
            log_assert(frame_data->polygon_collection->size <
                MAX_POLYGONS_PER_BUFFER);
        }
        
        if (particles_active < 1) {
            particle_effects[i].random_seed =
                (uint32_t)tok_rand() % RANDOM_SEQUENCE_SIZE;
        } else if (particle_effects[i].generate_light) {
            frame_data->light_collection->light_x[
                frame_data->light_collection->lights_size] =
                    particle_effects[i].zpolygon_gpu.xyz[0];
            frame_data->light_collection->light_y[
                frame_data->light_collection->lights_size] =
                    particle_effects[i].zpolygon_gpu.xyz[1];
            frame_data->light_collection->light_z[
                frame_data->light_collection->lights_size] =
                    particle_effects[i].zpolygon_gpu.xyz[2];
            
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
