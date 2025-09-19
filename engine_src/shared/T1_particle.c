#include "T1_particle.h"

#if T1_PARTICLES_ACTIVE == T1_ACTIVE

T1LineParticle * T1_particle_lineparticle_effects;
uint32_t T1_particle_lineparticle_effects_size;

static void construct_lineparticle_effect_no_zpoly(
    T1LineParticle * to_construct)
{
    to_construct->random_seed =
        (uint64_t)tok_rand_at_i(
            T1_platform_get_current_time_us() %
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
    
    to_construct->waypoints_size                    = 2;
    to_construct->waypoint_duration[0]              = 500000;
    to_construct->trail_delay                       = 300000;
    to_construct->particle_count                    = 500;
    to_construct->particle_zangle_variance_pct      = 314;
    to_construct->particle_rgb_variance_pct         = 15;
    to_construct->particle_scalefactor_variance_pct = 25;
}

T1LineParticle * T1_particle_lineparticle_get_next(void)
{
    for (uint32_t i = 0; i < T1_particle_lineparticle_effects_size; i++) {
        if (T1_particle_lineparticle_effects[i].deleted) {
            construct_lineparticle_effect_no_zpoly(&T1_particle_lineparticle_effects[i]);
            return &T1_particle_lineparticle_effects[i];
        }
    }
    
    T1_particle_lineparticle_effects_size += 1;
    log_assert(T1_particle_lineparticle_effects_size < LINEPARTICLE_EFFECTS_SIZE);
    
    construct_lineparticle_effect_no_zpoly(
        &T1_particle_lineparticle_effects[T1_particle_lineparticle_effects_size - 1]);
    
    return &T1_particle_lineparticle_effects[T1_particle_lineparticle_effects_size - 1];
}

T1LineParticle * T1_particle_lineparticle_get_next_with_zsprite(
    T1CPUzSprite * construct_with_zpolygon,
    T1GPUzSprite * construct_with_polygon_gpu)
{
    T1LineParticle * return_value = T1_particle_lineparticle_get_next();
    
    log_assert(T1_particle_lineparticle_effects_size < LINEPARTICLE_EFFECTS_SIZE);
    return_value->zpolygon_cpu = *construct_with_zpolygon;
    return_value->zpolygon_gpu = *construct_with_polygon_gpu;
    
    return return_value;
}

void T1_particle_lineparticle_commit(
    T1LineParticle * to_commit)
{
    log_assert(!to_commit->deleted);
    log_assert(to_commit->waypoints_size > 1);
    log_assert(to_commit->zpolygon_cpu.committed);
    log_assert(!to_commit->zpolygon_cpu.deleted);
    log_assert(to_commit->zpolygon_gpu.xyz_mult[0] > 0.0f);
    log_assert(to_commit->zpolygon_gpu.xyz_mult[1] > 0.0f);
    log_assert(to_commit->zpolygon_gpu.xyz_mult[2] > 0.0f);
    
    to_commit->committed = true;
    to_commit->random_seed = (uint32_t)tok_rand() %
        RANDOM_SEQUENCE_SIZE;
}

#define add_variance(x, variance, randnum, randnum2) if (variance > 0) { x += ((float)(randnum % variance) * 0.01f); x -= ((float)(randnum2 % variance) * 0.01f); }

void T1_particle_lineparticle_add_all_to_frame_data(
    T1GPUFrame * frame_data,
    uint64_t elapsed_us,
    const bool32_t alpha_blending)
{
    for (uint32_t i = 0; i < T1_particle_lineparticle_effects_size; i++) {
        if (
            T1_particle_lineparticle_effects[i].deleted ||
            !T1_particle_lineparticle_effects[i].committed ||
            !T1_particle_lineparticle_effects[i].zpolygon_cpu.committed ||
            T1_particle_lineparticle_effects[i].zpolygon_cpu.deleted ||
            T1_particle_lineparticle_effects[i].zpolygon_cpu.alpha_blending_enabled !=
                alpha_blending)
        {
            continue;
        }
        
        if (T1_particle_lineparticle_effects[i].wait_first > elapsed_us) {
            T1_particle_lineparticle_effects[i].wait_first -= elapsed_us;
            continue;
        } else if (T1_particle_lineparticle_effects[i].wait_first > 0) {
            T1_particle_lineparticle_effects[i].wait_first = 0;
            elapsed_us -= T1_particle_lineparticle_effects[i].wait_first;
        }
        
        T1_particle_lineparticle_effects[i].elapsed += elapsed_us;
        
        int32_t head_i =
            all_mesh_summaries[
                T1_particle_lineparticle_effects[i].zpolygon_cpu.mesh_id].vertices_head_i;
        log_assert(head_i >= 0);
        
        int32_t tail_i = head_i +
            all_mesh_summaries[
                T1_particle_lineparticle_effects[i].zpolygon_cpu.mesh_id].vertices_size;
        log_assert(tail_i >= 0);
        
        uint64_t lifetime_so_far = T1_particle_lineparticle_effects[i].elapsed;
        uint64_t total_lifetime = 0;
        for (
            uint32_t _ = 0;
            _ < T1_particle_lineparticle_effects[i].waypoints_size;
            _++)
        {
            total_lifetime += T1_particle_lineparticle_effects[i].waypoint_duration[_];
        }
        
        if (
            lifetime_so_far >
                total_lifetime + T1_particle_lineparticle_effects[i].trail_delay)
        {
            T1_particle_lineparticle_effects[i].deleted = true;
            continue;
        }
        
        uint64_t particle_rands[5];
        for (
            uint32_t particle_i = 0;
            particle_i < T1_particle_lineparticle_effects[i].particle_count;
            particle_i++)
        {
            uint64_t particle_delay =
                (T1_particle_lineparticle_effects[i].trail_delay /
                    T1_particle_lineparticle_effects[i].particle_count) *
                        particle_i;
            
            if (particle_delay > T1_particle_lineparticle_effects[i].elapsed) {
                continue;
            }
            
            particle_rands[0] = (uint64_t)tok_rand_at_i(
                (T1_particle_lineparticle_effects[i].random_seed + particle_i) %
                    RANDOM_SEQUENCE_SIZE);
            particle_rands[1] = (uint64_t)tok_rand_at_i(
                (T1_particle_lineparticle_effects[i].random_seed + particle_i + 37) %
                    RANDOM_SEQUENCE_SIZE);
            particle_rands[2] = (uint64_t)tok_rand_at_i(
                (T1_particle_lineparticle_effects[i].random_seed + particle_i + 51) %
                    RANDOM_SEQUENCE_SIZE);
            particle_rands[3] = (uint64_t)tok_rand_at_i(
                (T1_particle_lineparticle_effects[i].random_seed + particle_i + 237) %
                    RANDOM_SEQUENCE_SIZE);
            particle_rands[4] = (uint64_t)tok_rand_at_i(
                (T1_particle_lineparticle_effects[i].random_seed + particle_i + 414) %
                    RANDOM_SEQUENCE_SIZE);
            
            uint64_t delayed_lifetime_so_far =
                    lifetime_so_far > particle_delay ?
                        lifetime_so_far - particle_delay : 0;
            log_assert(
                delayed_lifetime_so_far <= T1_particle_lineparticle_effects[i].elapsed);
            
            uint64_t elapsed_in_this_waypoint = delayed_lifetime_so_far;
            uint32_t prev_i = 0;
            uint32_t next_i = 1;
            
            while (
                prev_i < MAX_LINEPARTICLE_DIRECTIONS &&
                elapsed_in_this_waypoint > T1_particle_lineparticle_effects[i].
                    waypoint_duration[prev_i])
            {
                elapsed_in_this_waypoint -= T1_particle_lineparticle_effects[i].
                    waypoint_duration[prev_i];
                prev_i += 1;
                next_i += 1;
            }
            
            if (
                next_i >= T1_particle_lineparticle_effects[i].waypoints_size ||
                prev_i >= MAX_LINEPARTICLE_DIRECTIONS)
            {
                continue;
            }
            
            frame_data->zsprite_list->polygons[
                frame_data->zsprite_list->size] =
                    T1_particle_lineparticle_effects[i].zpolygon_gpu;
            
            float next_multiplier =
                (float)elapsed_in_this_waypoint /
                    (float)T1_particle_lineparticle_effects[i].
                        waypoint_duration[prev_i];
            log_assert(next_multiplier < 1.01f);
            log_assert(next_multiplier > -0.01f);
            float prev_multiplier = 1.0f - next_multiplier;
            
            frame_data->zsprite_list->polygons[
                frame_data->zsprite_list->size].xyz[0] =
                    (prev_multiplier * T1_particle_lineparticle_effects[i].
                        waypoint_x[prev_i]) +
                    (next_multiplier * T1_particle_lineparticle_effects[i].
                        waypoint_x[next_i]);
            frame_data->zsprite_list->polygons[
                frame_data->zsprite_list->size].xyz[1] =
                    (prev_multiplier * T1_particle_lineparticle_effects[i].
                        waypoint_y[prev_i]) +
                    (next_multiplier * T1_particle_lineparticle_effects[i].
                        waypoint_y[next_i]);
            frame_data->zsprite_list->polygons[
                frame_data->zsprite_list->size].xyz[2] =
                    (prev_multiplier * T1_particle_lineparticle_effects[i].
                        waypoint_z[prev_i]) +
                    (next_multiplier * T1_particle_lineparticle_effects[i].
                        waypoint_z[next_i]);
            
            frame_data->zsprite_list->polygons[
                frame_data->zsprite_list->size].scale_factor =
                    (prev_multiplier * T1_particle_lineparticle_effects[i].
                        waypoint_scalefactor[prev_i]) +
                    (next_multiplier * T1_particle_lineparticle_effects[i].
                        waypoint_scalefactor[next_i]);
            add_variance(
                frame_data->zsprite_list->polygons[
                    frame_data->zsprite_list->size].scale_factor,
                T1_particle_lineparticle_effects[i].
                    particle_scalefactor_variance_pct,
                particle_rands[0],
                particle_rands[1]);
            
            log_assert(
                frame_data->zsprite_list->polygons[
                    frame_data->zsprite_list->size].xyz_offset[0] == 0);
            log_assert(
                frame_data->zsprite_list->polygons[
                    frame_data->zsprite_list->size].xyz_offset[0] == 0);
            log_assert(
                frame_data->zsprite_list->polygons[
                    frame_data->zsprite_list->size].xyz_offset[0] == 0);
            log_assert(
                frame_data->zsprite_list->polygons[
                    frame_data->zsprite_list->size].bonus_rgb[0] == 0);
            log_assert(
                frame_data->zsprite_list->polygons[
                    frame_data->zsprite_list->size].bonus_rgb[1] == 0);
            log_assert(
                frame_data->zsprite_list->polygons[
                    frame_data->zsprite_list->size].bonus_rgb[2] == 0);
            
            frame_data->zsprite_list->polygons[
                frame_data->zsprite_list->size].xyz_mult[0] =
                    T1_particle_lineparticle_effects[i].zpolygon_gpu.xyz_mult[0];
            frame_data->zsprite_list->polygons[
                frame_data->zsprite_list->size].xyz_mult[1] =
                    T1_particle_lineparticle_effects[i].zpolygon_gpu.xyz_mult[1];
            frame_data->zsprite_list->polygons[
                frame_data->zsprite_list->size].xyz_mult[2] =
                    T1_particle_lineparticle_effects[i].zpolygon_gpu.xyz_mult[2];
            
            add_variance(
                frame_data->zsprite_list->polygons[
                    frame_data->zsprite_list->size].xyz_angle[2],
                T1_particle_lineparticle_effects[i].particle_zangle_variance_pct,
                particle_rands[2],
                particle_rands[4]);
            
            for (
                int32_t vert_i = head_i;
                vert_i < (tail_i - 1);
                vert_i += 3)
            {
                for (uint32_t m = 0; m < 3; m++) {
                    frame_data->verts[frame_data->verts_size].
                        locked_vertex_i = (vert_i + (int32_t)m);
                    frame_data->verts[frame_data->verts_size].polygon_i =
                        (int)frame_data->zsprite_list->size;
                    
                    if (
                        frame_data->verts_size + 1 >=
                            MAX_VERTICES_PER_BUFFER)
                    {
                        return;
                    }
                    frame_data->verts_size += 1;
                }
            }
                
            if (
                frame_data->zsprite_list->size + 1 <
                    MAX_ZSPRITES_PER_BUFFER)
            {
                frame_data->zsprite_list->size += 1;
            } else {
                return;
            }
        }
    }
}

T1ParticleEffect * T1_particle_effects = NULL;
uint32_t T1_particle_effects_size = 0;

void T1_particle_effect_construct(
    T1ParticleEffect * to_construct)
{
    T1_std_memset(to_construct, 0, sizeof(T1ParticleEffect));
    
    T1zSpriteRequest poly_request;
    poly_request.cpu_data = &to_construct->zpolygon_cpu;
    poly_request.gpu_data = &to_construct->zpolygon_gpu;
    T1_zsprite_construct(&poly_request);
    
    to_construct->zsprite_id = -1;
    to_construct->zpolygon_cpu.mesh_id = BASIC_CUBE_MESH_ID;
    
    to_construct->zpolygon_cpu.committed = true;
    
    to_construct->zpolygon_gpu.base_mat.rgb_cap[0] = 1.0f;
    to_construct->zpolygon_gpu.base_mat.rgb_cap[1] = 1.0f;
    to_construct->zpolygon_gpu.base_mat.rgb_cap[2] = 1.0f;
    to_construct->zpolygon_gpu.scale_factor      = 1.0f;
    to_construct->zpolygon_gpu.xyz_mult[0] = 0.01f;
    to_construct->zpolygon_gpu.xyz_mult[1] = 0.01f;
    to_construct->zpolygon_gpu.xyz_mult[2] = 0.01f;
    to_construct->zpolygon_gpu.ignore_lighting = true;
    
    to_construct->random_seed = (uint32_t)
        tok_rand() % (RANDOM_SEQUENCE_SIZE - 100);
    to_construct->spawns_per_sec = 200;
    to_construct->verts_per_particle = 3;
    to_construct->lifespan = 2000000;
}

T1ParticleEffect * T1_particle_get_next(void) {
    T1ParticleEffect * return_value = NULL;
    
    for (uint32_t i = 0; i < T1_particle_effects_size; i++) {
        if (T1_particle_effects[i].deleted) {
            return_value = &T1_particle_effects[i];
            break;
        }
    }
    
    if (return_value == NULL) {
        log_assert(T1_particle_effects_size + 1 <
            PARTICLE_EFFECTS_SIZE);
        return_value = &T1_particle_effects[T1_particle_effects_size];
        T1_particle_effects_size += 1;
    }
    
    T1_particle_effect_construct(return_value);
    return return_value;
}

void T1_particle_commit(T1ParticleEffect * to_request)
{
    if (!engine_globals->clientlogic_early_startup_finished) {
        log_dump_and_crash(
            "You can't commit particle effects during early startup.");
        return;
    }
    
    log_assert(
        to_request->zpolygon_cpu.mesh_id >= 0);
    log_assert(
        to_request->zpolygon_cpu.visible);
    
    log_assert(
        all_mesh_summaries[to_request->zpolygon_cpu.mesh_id].
            materials_size > 0);
    
    // Reminder: The particle effect is not committed, but the zpoly should be
    log_assert(to_request->zpolygon_cpu.committed);
    
    log_assert(
        (uint32_t)to_request->zpolygon_cpu.mesh_id < all_mesh_summaries_size);
    log_assert(!to_request->deleted);
    
    // Reminder: The particle effect is not committed, but the zpoly should be
    log_assert(!to_request->committed);
    
    log_assert(to_request->lifespan > 0);
    log_assert(to_request->elapsed == 0);
    log_assert(to_request->spawns_per_sec > 0);
    log_assert(to_request->verts_per_particle > 0);
    
    for (uint32_t _ = 0; _ < 3; _++) {
        if (to_request->zpolygon_gpu.xyz_mult[0] < 0.00001f) {
            to_request->zpolygon_gpu.xyz_mult[0] = 0.00001f;
        }
    }
    
    to_request->committed = true;
}

void T1_particle_delete(int32_t with_object_id) {
    for (uint32_t i = 0; i < T1_particle_effects_size; i++) {
        if (T1_particle_effects[i].zsprite_id == with_object_id) {
            T1_particle_effects[i].deleted = true;
        }
    }
    
    while (
        T1_particle_effects_size > 0 &&
        T1_particle_effects[T1_particle_effects_size - 1].deleted)
    {
        T1_particle_effects_size -= 1;
    }
}

void T1_particle_add_all_to_frame_data(
    T1GPUFrame * frame_data,
    uint64_t elapsed_us,
    const bool32_t alpha_blending)
{
    uint64_t spawns_in_duration;
    uint64_t interval_between_spawns;
    uint64_t spawn_lifetime_so_far;
    
    for (
        uint32_t i = 0;
        i < T1_particle_effects_size;
        i++)
    {
        if (T1_particle_effects[i].deleted ||
            !T1_particle_effects[i].committed ||
            T1_particle_effects[i].zpolygon_cpu.alpha_blending_enabled !=
                alpha_blending ||
            frame_data->zsprite_list->size >= MAX_ZSPRITES_PER_BUFFER)
        {
            continue;
        }
        
        T1_particle_effects[i].elapsed += elapsed_us;
        
        while (T1_particle_effects[i].verts_per_particle % 3 != 0) {
            T1_particle_effects[i].verts_per_particle += 1;
        }
        
        if (T1_particle_effects[i].elapsed >
            (T1_particle_effects[i].lifespan +
                T1_particle_effects[i].pause_per_spawn))
        {
            if (T1_particle_effects[i].loops == 1) {
                T1_particle_effects[i].deleted = true;
                continue;
            }
            
            if (T1_particle_effects[i].loops > 1) {
                T1_particle_effects[i].loops -= 1;
            }
            
            T1_particle_effects[i].elapsed = 0;
        }
        
        spawns_in_duration =
            (T1_particle_effects[i].lifespan *
                T1_particle_effects[i].spawns_per_sec) /
                    1000000;
        interval_between_spawns =
            T1_particle_effects[i].pause_per_spawn;
        
        float particles_active = 0;
        
        int32_t vert_head_i = T1_particle_effects[i].shattered ?
            all_mesh_summaries[
                T1_particle_effects[i].zpolygon_cpu.mesh_id].
                    shattered_vertices_head_i :
            all_mesh_summaries[
                T1_particle_effects[i].zpolygon_cpu.mesh_id].
                    vertices_head_i;
        int32_t verts_size = T1_particle_effects[i].shattered ?
            all_mesh_summaries[
                T1_particle_effects[i].zpolygon_cpu.mesh_id].
                    shattered_vertices_size :
            all_mesh_summaries[
                T1_particle_effects[i].zpolygon_cpu.mesh_id].
                    vertices_size;
        if (verts_size < (int32_t)T1_particle_effects[i].verts_per_particle) {
            return;
        }
        int32_t queue_vert_i = 0;
        
        for (
            uint32_t spawn_i = 0;
            spawn_i < spawns_in_duration;
            spawn_i++)
        {
            uint64_t rand_i =
                (T1_particle_effects[i].random_seed
                    + (spawn_i * 9)) %
                    (FLOAT_SEQUENCE_SIZE - 256);
            
            if (T1_particle_effects[i].elapsed <
                (spawn_i * interval_between_spawns))
            {
                continue;
            }
            
            spawn_lifetime_so_far =
                (T1_particle_effects[i].elapsed -
                (spawn_i * interval_between_spawns));
            
            if (spawn_lifetime_so_far > T1_particle_effects[i].lifespan)
            {
                continue;
            }
            
            for (
                int32_t _ = 0;
                _ < (int32_t)T1_particle_effects[i].verts_per_particle;
                _++)
            {
                int32_t next_vert_i = vert_head_i +
                    (queue_vert_i % verts_size);
                queue_vert_i += 1;
                
                log_assert(next_vert_i >= 0);
                log_assert(next_vert_i < (int32_t)all_mesh_vertices->size);
                
                frame_data->verts[frame_data->verts_size].
                    locked_vertex_i = next_vert_i;
                frame_data->verts[frame_data->verts_size].polygon_i =
                    (int)frame_data->zsprite_list->size;
                
                frame_data->verts_size += 1;
                log_assert(
                    frame_data->verts_size < MAX_VERTICES_PER_BUFFER);
            }
            
            log_assert(
                frame_data->zsprite_list->size < MAX_ZSPRITES_PER_BUFFER);
            T1_std_memcpy(
                /* void * dst: */
                    frame_data->zsprite_list->polygons +
                        frame_data->zsprite_list->size,
                /* const void * src: */
                    &T1_particle_effects[i].zpolygon_gpu,
                /* size_t n: */
                    sizeof(T1GPUzSprite));
            
            log_assert(
                frame_data->zsprite_list->size < MAX_ZSPRITES_PER_BUFFER);
            
            float * initial_random_add_1_at = (float *)&T1_particle_effects[i].
                init_rand_add[0];
            float * initial_random_add_2_at = (float *)&T1_particle_effects[i].
                init_rand_add[1];
            
            float * pertime_add_at = (float *)&T1_particle_effects[i].
                pertime_add;
            float * perexptime_add_at = (float *)&T1_particle_effects[i].
                perexptime_add;
            float * pertime_random_add_1_at = (float *)&T1_particle_effects[i].
                pertime_rand_add[0];
            float * pertime_random_add_2_at = (float *)&T1_particle_effects[i].
                pertime_rand_add[1];
            float * recipient_at = (float *)&frame_data->zsprite_list->
                polygons[frame_data->zsprite_list->size];
            
            float one_million = 1000000.0f;
            float exponential_divisor = 1000.0f;
            float one = 1.0f;
            SIMD_FLOAT simdf_one_million = simd_set1_float(one_million);
            float fspawn_lifetime_so_far = (float)spawn_lifetime_so_far;
            SIMD_FLOAT simdf_lifetime = simd_set1_float(fspawn_lifetime_so_far);
            SIMD_FLOAT simdf_lifetime_exp = simd_div_floats(
                simdf_lifetime,
                simd_set1_float(exponential_divisor));
            simdf_lifetime_exp = simd_max_floats(
                simdf_lifetime_exp,
                simd_set1_float(one));
            simdf_lifetime_exp = simd_mul_floats(
                simdf_lifetime_exp,
                simdf_lifetime_exp);
            
            for (
                uint32_t j = 0;
                j < (sizeof(T1GPUzSprite) / sizeof(float));
                j += SIMD_FLOAT_LANES)
            {
                // We expect padding to prevent out of bounds ops
                log_assert((j + SIMD_FLOAT_LANES) * sizeof(float) <=
                    sizeof(T1GPUzSprite));
                
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
                
                // Convert per second values to per microsecond (us) effect
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
                    ((rand_i + ((j/SIMD_FLOAT_LANES) *
                        (SIMD_FLOAT_LANES * 4)))
                            + 96)% (RANDOM_SEQUENCE_SIZE-3));
                simdf_initial_add = simd_load_floats(
                    initial_random_add_2_at + j);
                simdf_initial_add = simd_mul_floats(
                    simdf_initial_add, simdf_rand);
                recip = simd_add_floats(recip, simdf_initial_add);
                
                log_assert((ptrdiff_t)(recipient_at + j) %
                    (long)(SIMD_FLOAT_LANES * sizeof(float)) == 0);
                simd_store_floats((recipient_at + j), recip);
            }
            
            if (frame_data->zsprite_list->polygons[
                frame_data->zsprite_list->size].scale_factor < 0.01f)
            {
                frame_data->zsprite_list->polygons[
                    frame_data->zsprite_list->size].scale_factor = 0.001f;
            }
            
            particles_active += (1.0f *
                frame_data->zsprite_list->polygons[
                    frame_data->zsprite_list->size].scale_factor);
            
            frame_data->zsprite_list->size += 1;
            log_assert(
                frame_data->zsprite_list->size <
                    MAX_ZSPRITES_PER_BUFFER);
        }
        
        if (particles_active < 1) {
            T1_particle_effects[i].random_seed =
                (uint32_t)tok_rand() % RANDOM_SEQUENCE_SIZE;
        }
        
        if (T1_particle_effects[i].cast_light) {
            frame_data->lights[frame_data->lights_size].xyz[0]
                 =
                    T1_particle_effects[i].zpolygon_gpu.xyz[0];
            frame_data->lights[
                frame_data->lights_size].xyz[1] =
                    T1_particle_effects[i].zpolygon_gpu.xyz[1];
            frame_data->lights[
                frame_data->lights_size].xyz[2] =
                    T1_particle_effects[i].zpolygon_gpu.xyz[2];
            
            frame_data->lights[
                frame_data->lights_size].rgb[0] =
                    T1_particle_effects[i].light_rgb[0];
            frame_data->lights[
                frame_data->lights_size].rgb[1] =
                    T1_particle_effects[i].light_rgb[1];
            frame_data->lights[
                frame_data->lights_size].rgb[2] =
                    T1_particle_effects[i].light_rgb[2];
            
            frame_data->lights[
                frame_data->lights_size].reach =
                    T1_particle_effects[i].light_reach;
            
            float light_strength =
                particles_active / (float)spawns_in_duration;
            
            log_assert(light_strength < 1.25f);
            log_assert(light_strength >= 0.0f);
            
            frame_data->lights[
                frame_data->lights_size].rgb[0] =
                    0.01f *
                    light_strength *
                    T1_particle_effects[i].light_rgb[0];
            frame_data->lights[
                frame_data->lights_size].rgb[1] =
                    0.01f *
                    light_strength *
                    T1_particle_effects[i].light_rgb[1];
            frame_data->lights[
                frame_data->lights_size].rgb[2] =
                    0.01f *
                    light_strength *
                    T1_particle_effects[i].light_rgb[2];
            frame_data->lights[
                frame_data->lights_size].diffuse =
                    light_strength *
                    T1_particle_effects[i].light_strength;
            frame_data->lights[
                frame_data->lights_size].reach =
                    light_strength *
                    T1_particle_effects[i].light_reach;
            
            frame_data->lights_size += 1;
        }
    }
}

#elif T1_PARTICLES_ACTIVE == T1_INACTIVE
#else
#error
#endif // PARTICLES_ACTIVE
