#include "renderer.h"

static uint32_t renderer_initialized = false;

void init_renderer() {
    renderer_initialized = true;
    
    camera.x       = 0.0f;
    camera.y       = 0.0f;
    camera.z       = 0.0f;
    camera.x_angle = 0.0f;
    camera.y_angle = 0.0f;
    camera.z_angle = 0.0f;
}

inline static void zpolygons_to_triangles(
    GPUVertex * next_gpu_workload,
    uint32_t * next_workload_size,
    GPUPolygonCollection * gpu_polygons)
{
    assert(gpu_polygons->size == 0);
    assert(*next_workload_size == 0);
    
    for (
        int32_t zp_i = 0;
        zp_i < zpolygons_to_render_size;
        zp_i++)
    {
        if (
            zpolygons_to_render[zp_i].deleted ||
            !zpolygons_to_render[zp_i].visible ||
            zpolygons_to_render[zp_i].mesh_id < 0 ||
            (uint32_t)zpolygons_to_render[zp_i].mesh_id >=
                all_mesh_summaries_size)
        {
            continue;
        }
        
        assert(gpu_polygons->size < MAX_POLYGONS_PER_BUFFER);
        gpu_polygons->xyz[gpu_polygons->size][0] =
            zpolygons_to_render[zp_i].x;
        gpu_polygons->xyz[gpu_polygons->size][1] =
            zpolygons_to_render[zp_i].y;
        gpu_polygons->xyz[gpu_polygons->size][2] =
            zpolygons_to_render[zp_i].z;
        gpu_polygons->xyz_angle[gpu_polygons->size][0] =
            zpolygons_to_render[zp_i].x_angle;
        gpu_polygons->xyz_angle[gpu_polygons->size][1] =
            zpolygons_to_render[zp_i].y_angle;
        gpu_polygons->xyz_angle[gpu_polygons->size][2] =
            zpolygons_to_render[zp_i].z_angle;
        gpu_polygons->xyz_multiplier[gpu_polygons->size][0] =
            zpolygons_to_render[zp_i].x_multiplier;
        gpu_polygons->xyz_multiplier[gpu_polygons->size][1] =
            zpolygons_to_render[zp_i].y_multiplier;
        gpu_polygons->xyz_multiplier[gpu_polygons->size][2] =
            zpolygons_to_render[zp_i].z_multiplier;
        gpu_polygons->xy_offset[gpu_polygons->size][0] =
            zpolygons_to_render[zp_i].x_offset;
        gpu_polygons->xy_offset[gpu_polygons->size][1] =
            zpolygons_to_render[zp_i].y_offset;
        gpu_polygons->bonus_rgb[gpu_polygons->size][0] =
            zpolygons_to_render[zp_i].rgb_bonus[0];
        gpu_polygons->bonus_rgb[gpu_polygons->size][1] =
            zpolygons_to_render[zp_i].rgb_bonus[1];
        gpu_polygons->bonus_rgb[gpu_polygons->size][2] =
            zpolygons_to_render[zp_i].rgb_bonus[2];
        gpu_polygons->scale_factor[gpu_polygons->size] =
            zpolygons_to_render[zp_i].scale_factor;
        gpu_polygons->ignore_lighting[gpu_polygons->size] =
            zpolygons_to_render[zp_i].ignore_lighting;
        gpu_polygons->ignore_camera[gpu_polygons->size] =
            zpolygons_to_render[zp_i].ignore_camera;
        log_assert(gpu_polygons->size < MAX_POLYGONS_PER_BUFFER);
        
        int32_t mesh_id = zpolygons_to_render[zp_i].mesh_id;
        log_assert(mesh_id >= 0);
        log_assert(mesh_id < (int32_t)all_mesh_summaries_size);
        
        int32_t vert_tail_i =
            all_mesh_summaries[mesh_id].vertices_head_i +
                all_mesh_summaries[mesh_id].vertices_size;
        
        // TODO: copying vertices should no longer be needed every frame, stop
        for (
            int32_t vert_i = all_mesh_summaries[mesh_id].vertices_head_i;
            vert_i < vert_tail_i;
            vert_i++)
        {
            (*next_gpu_workload).locked_vertex_i = vert_i;
            (*next_gpu_workload).polygon_i = zp_i;
            next_gpu_workload += 1;
        }
        *next_workload_size += all_mesh_summaries[mesh_id].vertices_size;
        
        gpu_polygons->size += 1;
    }
    
    assert(gpu_polygons->size <= zpolygons_to_render_size);
}

void hardware_render(
    GPUDataForSingleFrame * frame_data,
    uint64_t elapsed_nanoseconds)
{
    (void)elapsed_nanoseconds;
    
    if (renderer_initialized != true) {
        log_append("renderer not initialized, aborting...\n");
        return;
    }
    
    if (
        frame_data == NULL ||
        frame_data->vertices == NULL)
    {
        log_append("ERROR: platform layer didnt pass recipients\n");
        return;
    }
    
    // TODO: will need re-iplementation after we refactor the gpu buffers
    #if 0
    #ifndef LOGGER_IGNORE_ASSERTS
    if (window_globals->visual_debug_mode) {
        /*
        draw the ray that's used for finding touchables when clicking as a
        triangle
        */
        for (uint32_t m = 0; m < 3; m++) {
            uint32_t next_wl_i = *next_workload_size + m;
            
            next_gpu_workload[next_wl_i].x =
                window_globals->visual_debug_ray_origin_direction[(m*3) + 0];
            next_gpu_workload[next_wl_i].y =
                window_globals->visual_debug_ray_origin_direction[(m*3) + 1];
            next_gpu_workload[next_wl_i].z =
                window_globals->visual_debug_ray_origin_direction[(m*3) + 2];
            // next_gpu_workload[next_wl_i].parent_x = 0.0f;
            // next_gpu_workload[next_wl_i].parent_y = 0.0f;
            // next_gpu_workload[next_wl_i].parent_z = 0.0f;
            next_gpu_workload[next_wl_i].texturearray_i = -1;
            next_gpu_workload[next_wl_i].texture_i = -1;
            next_gpu_workload[next_wl_i].RGBA[0] = 1.0f;
            next_gpu_workload[next_wl_i].RGBA[1] = 1.0f;
            next_gpu_workload[next_wl_i].RGBA[2] = 1.0f;
            next_gpu_workload[next_wl_i].RGBA[3] = 0.75f;
            // next_gpu_workload[next_wl_i].ignore_lighting = true;
            // next_gpu_workload[next_wl_i].scale_factor = 1.0f;
            // next_gpu_workload[next_wl_i].ignore_camera = true;
            // next_gpu_workload[next_wl_i].touchable_id = -1;
            next_gpu_workload[next_wl_i].x_angle = 0.0f;
            next_gpu_workload[next_wl_i].y_angle = 0.0f;
            next_gpu_workload[next_wl_i].z_angle = 0.0f;
        }
        *next_workload_size += 3;
    }
    #endif
    #endif
    
    log_assert(zpolygons_to_render_size < MAX_POLYGONS_PER_BUFFER);
    
    zpolygons_to_triangles(
        frame_data->vertices,
        &frame_data->vertices_size,
        frame_data->polygon_collection);
    
    if (application_running) {
        // TODO: re-implement particle effects
        //        add_particle_effects_to_workload(
        //            next_gpu_workload,
        //            next_workload_size,
        //            lights_for_gpu,
        //            elapsed_nanoseconds);
        //
        //        add_shatter_effects_to_workload(
        //            next_gpu_workload,
        //            next_workload_size,
        //            lights_for_gpu,
        //            elapsed_nanoseconds);
    }
}
