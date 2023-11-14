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

inline static void add_point_vertex(
    GPUDataForSingleFrame * frame_data,
    float x,
    float y,
    float z,
    bool32_t ignore_camera)
{
    frame_data->polygon_collection->xyz[
        frame_data->polygon_collection->size][0] =
            x;
    frame_data->polygon_collection->xyz[
        frame_data->polygon_collection->size][1] =
            y;
    frame_data->polygon_collection->xyz[
        frame_data->polygon_collection->size][2] =
            z;
    frame_data->polygon_collection->ignore_lighting[
        frame_data->polygon_collection->size] = true;
    frame_data->polygon_collection->ignore_camera[
        frame_data->polygon_collection->size] = ignore_camera;
    frame_data->polygon_collection->bonus_rgb[
        frame_data->polygon_collection->size][0] = 0.0f;
    frame_data->polygon_collection->bonus_rgb[
        frame_data->polygon_collection->size][1] = 0.0f;
    frame_data->polygon_collection->bonus_rgb[
        frame_data->polygon_collection->size][2] = 0.0f;
    
    frame_data->polygon_collection->scale_factor[
        frame_data->polygon_collection->size] = 1.0f;
    
    frame_data->polygon_collection->xy_offset[
        frame_data->polygon_collection->size][0] = 0.0f;
    frame_data->polygon_collection->xy_offset[
        frame_data->polygon_collection->size][1] = 0.0f;
    
    frame_data->polygon_collection->xyz_angle[
        frame_data->polygon_collection->size][0] = 0.0f;
    frame_data->polygon_collection->xyz_angle[
        frame_data->polygon_collection->size][1] = 0.0f;
    frame_data->polygon_collection->xyz_angle[
        frame_data->polygon_collection->size][2] = 0.0f;
    
    frame_data->polygon_collection->xyz_multiplier[
        frame_data->polygon_collection->size][0] = 1.0f;
    frame_data->polygon_collection->xyz_multiplier[
        frame_data->polygon_collection->size][1] = 1.0f;
    frame_data->polygon_collection->xyz_multiplier[
        frame_data->polygon_collection->size][2] = 1.0f;
    
    frame_data->vertices[frame_data->vertices_size].texturearray_i =
        -1;
    frame_data->vertices[frame_data->vertices_size].texture_i = -1;
    frame_data->vertices[frame_data->vertices_size].polygon_i =
        (int)frame_data->polygon_collection->size;
    frame_data->vertices[frame_data->vertices_size].color[0] = 0.0f;
    frame_data->vertices[frame_data->vertices_size].color[1] = 1.0f;
    frame_data->vertices[frame_data->vertices_size].color[2] = 1.0f;
    frame_data->vertices[frame_data->vertices_size].color[3] = 1.0f;
    frame_data->vertices[frame_data->vertices_size].locked_vertex_i =
        all_mesh_summaries[2].vertices_head_i;
    
    frame_data->polygon_collection->size += 1;
    frame_data->vertices_size += 1;
}

inline static void zpolygon_hitboxes_to_lines(
    GPUDataForSingleFrame * frame_data)
{
    for (uint32_t zp_i = 0; zp_i < zpolygons_to_render_size; zp_i++) {
        if (zpolygons_to_render[zp_i].touchable_id >= 0) {
            float left =
                zpolygons_to_render[zp_i].x -
                (zpolygons_to_render[zp_i].hitbox_width / 2);
            float right =
                zpolygons_to_render[zp_i].x +
                (zpolygons_to_render[zp_i].hitbox_width / 2);
            float top =
                zpolygons_to_render[zp_i].y +
                (zpolygons_to_render[zp_i].hitbox_height / 2);
            float bottom =
                zpolygons_to_render[zp_i].y -
                (zpolygons_to_render[zp_i].hitbox_height / 2);
            float front =
                zpolygons_to_render[zp_i].z -
                (zpolygons_to_render[zp_i].hitbox_depth / 2);
            float back =
                zpolygons_to_render[zp_i].z +
                (zpolygons_to_render[zp_i].hitbox_depth / 2);
            
            zVertex topleftfront;
            topleftfront.x = left;
            topleftfront.y = top;
            topleftfront.z = front;
            x_rotate_zvertex(&topleftfront, -zpolygons_to_render[zp_i].x_angle);
            y_rotate_zvertex(&topleftfront, -zpolygons_to_render[zp_i].y_angle);
            z_rotate_zvertex(&topleftfront, -zpolygons_to_render[zp_i].z_angle);
            
            zVertex rightbottomback;
            rightbottomback.x = right;
            rightbottomback.y = bottom;
            rightbottomback.z = back;
            x_rotate_zvertex(
                &rightbottomback, -zpolygons_to_render[zp_i].x_angle);
            y_rotate_zvertex(
                &rightbottomback, -zpolygons_to_render[zp_i].y_angle);
            z_rotate_zvertex(
                &rightbottomback, -zpolygons_to_render[zp_i].z_angle);
            
            // left top front -> right top front
            add_point_vertex(
                frame_data,
                topleftfront.x, topleftfront.y, topleftfront.z,
                zpolygons_to_render[zp_i].ignore_camera);
            add_point_vertex(
                frame_data,
                rightbottomback.x, topleftfront.y, topleftfront.z,
                zpolygons_to_render[zp_i].ignore_camera);
            
            // left bottom front -> right bottom front
            add_point_vertex(
                frame_data,
                topleftfront.x, rightbottomback.y, topleftfront.z,
                zpolygons_to_render[zp_i].ignore_camera);
            add_point_vertex(
                frame_data,
                rightbottomback.x, rightbottomback.y, topleftfront.z,
                zpolygons_to_render[zp_i].ignore_camera);
            
            // left top back -> right top back
            add_point_vertex(
                frame_data,
                topleftfront.x, topleftfront.y, rightbottomback.z,
                zpolygons_to_render[zp_i].ignore_camera);
            add_point_vertex(
                frame_data,
                rightbottomback.x, topleftfront.y, rightbottomback.z,
                zpolygons_to_render[zp_i].ignore_camera);
            
            // left bottom back -> right bottom back
            add_point_vertex(
                frame_data,
                topleftfront.x, rightbottomback.y, rightbottomback.z,
                zpolygons_to_render[zp_i].ignore_camera);
            add_point_vertex(
                frame_data,
                rightbottomback.x, rightbottomback.y, rightbottomback.z,
                zpolygons_to_render[zp_i].ignore_camera);
            
            // left top front -> left top back
            add_point_vertex(
                frame_data,
                topleftfront.x, topleftfront.y, topleftfront.z,
                zpolygons_to_render[zp_i].ignore_camera);
            add_point_vertex(
                frame_data,
                topleftfront.x, topleftfront.y, rightbottomback.z,
                zpolygons_to_render[zp_i].ignore_camera);
            
            // right top front -> right top back
            add_point_vertex(
                frame_data,
                rightbottomback.x, topleftfront.y, topleftfront.z,
                zpolygons_to_render[zp_i].ignore_camera);
            add_point_vertex(
                frame_data,
                rightbottomback.x, topleftfront.y, rightbottomback.z,
                zpolygons_to_render[zp_i].ignore_camera);
            
            // left bottom front -> left bottom back
            add_point_vertex(
                frame_data,
                topleftfront.x, rightbottomback.y, topleftfront.z,
                zpolygons_to_render[zp_i].ignore_camera);
            add_point_vertex(
                frame_data,
                topleftfront.x, rightbottomback.y, rightbottomback.z,
                zpolygons_to_render[zp_i].ignore_camera);
            
            // right bottom front -> right bottom back
            add_point_vertex(
                frame_data,
                rightbottomback.x, rightbottomback.y, topleftfront.z,
                zpolygons_to_render[zp_i].ignore_camera);
            add_point_vertex(
                frame_data,
                rightbottomback.x, rightbottomback.y, rightbottomback.z,
                zpolygons_to_render[zp_i].ignore_camera);
            
            // left top front -> left bottom front
            add_point_vertex(
                frame_data,
                topleftfront.x, topleftfront.y, topleftfront.z,
                zpolygons_to_render[zp_i].ignore_camera);
            add_point_vertex(
                frame_data,
                topleftfront.x, rightbottomback.y, topleftfront.z,
                zpolygons_to_render[zp_i].ignore_camera);
            
            // right top front -> right bottom front
            add_point_vertex(
                frame_data,
                rightbottomback.x, topleftfront.y, topleftfront.z,
                zpolygons_to_render[zp_i].ignore_camera);
            add_point_vertex(
                frame_data,
                rightbottomback.x, rightbottomback.y, topleftfront.z,
                zpolygons_to_render[zp_i].ignore_camera);
            
            // left top back -> left bottom back
            add_point_vertex(
                frame_data,
                topleftfront.x, topleftfront.y, rightbottomback.z,
                zpolygons_to_render[zp_i].ignore_camera);
            add_point_vertex(
                frame_data,
                topleftfront.x, rightbottomback.y, rightbottomback.z,
                zpolygons_to_render[zp_i].ignore_camera);
            
            // right top back -> right bottom back
            add_point_vertex(
                frame_data,
                rightbottomback.x, topleftfront.y, rightbottomback.z,
                zpolygons_to_render[zp_i].ignore_camera);
            add_point_vertex(
                frame_data,
                rightbottomback.x, rightbottomback.y, rightbottomback.z,
                zpolygons_to_render[zp_i].ignore_camera);
        }
    }
}

inline static void zpolygons_to_triangles(
    GPUDataForSingleFrame * frame_data)
{
    uint32_t * next_workload_size = &frame_data->vertices_size;
    GPUPolygonCollection * gpu_polygons = frame_data->polygon_collection;
    
    assert(gpu_polygons->size == 0);
    assert(*next_workload_size == 0);
    
    for (
        int32_t cpu_zp_i = 0;
        cpu_zp_i < (int32_t)zpolygons_to_render_size;
        cpu_zp_i++)
    {
        if (
            zpolygons_to_render[cpu_zp_i].deleted ||
            !zpolygons_to_render[cpu_zp_i].visible ||
            zpolygons_to_render[cpu_zp_i].mesh_id < 0 ||
            (uint32_t)zpolygons_to_render[cpu_zp_i].mesh_id >=
                all_mesh_summaries_size)
        {
            continue;
        }
        
        assert(gpu_polygons->size <= zpolygons_to_render_size);
        assert(zpolygons_to_render_size < MAX_POLYGONS_PER_BUFFER);
        assert(gpu_polygons->size < MAX_POLYGONS_PER_BUFFER);
        
        gpu_polygons->xyz[gpu_polygons->size][0] =
            zpolygons_to_render[cpu_zp_i].x;
        gpu_polygons->xyz[gpu_polygons->size][1] =
            zpolygons_to_render[cpu_zp_i].y;
        gpu_polygons->xyz[gpu_polygons->size][2] =
            zpolygons_to_render[cpu_zp_i].z;
        gpu_polygons->xyz_angle[gpu_polygons->size][0] =
            zpolygons_to_render[cpu_zp_i].x_angle;
        gpu_polygons->xyz_angle[gpu_polygons->size][1] =
            zpolygons_to_render[cpu_zp_i].y_angle;
        gpu_polygons->xyz_angle[gpu_polygons->size][2] =
            zpolygons_to_render[cpu_zp_i].z_angle;
        gpu_polygons->xyz_multiplier[gpu_polygons->size][0] =
            zpolygons_to_render[cpu_zp_i].x_multiplier;
        gpu_polygons->xyz_multiplier[gpu_polygons->size][1] =
            zpolygons_to_render[cpu_zp_i].y_multiplier;
        gpu_polygons->xyz_multiplier[gpu_polygons->size][2] =
            zpolygons_to_render[cpu_zp_i].z_multiplier;
        gpu_polygons->xy_offset[gpu_polygons->size][0] =
            zpolygons_to_render[cpu_zp_i].x_offset;
        gpu_polygons->xy_offset[gpu_polygons->size][1] =
            zpolygons_to_render[cpu_zp_i].y_offset;
        gpu_polygons->bonus_rgb[gpu_polygons->size][0] =
            zpolygons_to_render[cpu_zp_i].rgb_bonus[0];
        gpu_polygons->bonus_rgb[gpu_polygons->size][1] =
            zpolygons_to_render[cpu_zp_i].rgb_bonus[1];
        gpu_polygons->bonus_rgb[gpu_polygons->size][2] =
            zpolygons_to_render[cpu_zp_i].rgb_bonus[2];
        gpu_polygons->scale_factor[gpu_polygons->size] =
            zpolygons_to_render[cpu_zp_i].scale_factor;
        gpu_polygons->ignore_lighting[gpu_polygons->size] =
            zpolygons_to_render[cpu_zp_i].ignore_lighting;
        gpu_polygons->ignore_camera[gpu_polygons->size] =
            zpolygons_to_render[cpu_zp_i].ignore_camera;
        log_assert(gpu_polygons->size < MAX_POLYGONS_PER_BUFFER);
        
        int32_t mesh_id = zpolygons_to_render[cpu_zp_i].mesh_id;
        log_assert(mesh_id >= 0);
        log_assert(mesh_id < (int32_t)all_mesh_summaries_size);
        
        int32_t vert_tail_i =
            all_mesh_summaries[mesh_id].vertices_head_i +
                all_mesh_summaries[mesh_id].vertices_size;
        assert(vert_tail_i < MAX_VERTICES_PER_BUFFER);
        
        for (
            int32_t vert_i = all_mesh_summaries[mesh_id].vertices_head_i;
            vert_i < vert_tail_i;
            vert_i++)
        {
            frame_data->vertices[*next_workload_size].locked_vertex_i = vert_i;
            frame_data->vertices[*next_workload_size].polygon_i =
                (int)gpu_polygons->size; // this is not the same as cpu_zp_i!!!
            
            uint32_t mat_i = all_mesh_vertices[vert_i].parent_material_i;
            log_assert(
                mat_i < zpolygons_to_render[cpu_zp_i].vertex_materials_size);
            
            frame_data->vertices[*next_workload_size].color[0] =
                zpolygons_to_render[cpu_zp_i].vertex_materials[mat_i].color[0];
            frame_data->vertices[*next_workload_size].color[1] =
                zpolygons_to_render[cpu_zp_i].vertex_materials[mat_i].color[1];
            frame_data->vertices[*next_workload_size].color[2] =
                zpolygons_to_render[cpu_zp_i].vertex_materials[mat_i].color[2];
            frame_data->vertices[*next_workload_size].color[3] =
                zpolygons_to_render[cpu_zp_i].vertex_materials[mat_i].color[3];
            frame_data->vertices[*next_workload_size].texture_i =
                zpolygons_to_render[cpu_zp_i].vertex_materials[mat_i].texture_i;
            frame_data->vertices[*next_workload_size].texturearray_i =
                zpolygons_to_render[cpu_zp_i].vertex_materials[mat_i].
                    texturearray_i;
            *next_workload_size += 1;
            assert(*next_workload_size < MAX_VERTICES_PER_BUFFER);
        }
        
        gpu_polygons->size += 1;
    }
    
    frame_data->first_line_i = frame_data->vertices_size;
    
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
    
    log_assert(zpolygons_to_render_size < MAX_POLYGONS_PER_BUFFER);
    
    zpolygons_to_triangles(
        frame_data);
    
    if (window_globals->visual_debug_mode) {
        zpolygon_hitboxes_to_lines(
            frame_data);
    }
    
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
    
    if (window_globals->wireframe_mode) {
        frame_data->first_line_i = 0;
    }
}
