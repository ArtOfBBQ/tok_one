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

static bool32_t is_last_clicked = false;

inline static void add_point_vertex(
    GPUDataForSingleFrame * frame_data,
    float x,
    float y,
    float z,
    bool32_t ignore_camera)
{
    if (frame_data->polygon_collection->size >= MAX_POLYGONS_PER_BUFFER) {
        return;
    }
    
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz[0] =
            x;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz[1] =
            y;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz[2] =
            z;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].ignore_lighting = true;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].ignore_camera = ignore_camera;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].bonus_rgb[0] = 0.0f;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].bonus_rgb[1] = 0.0f;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].bonus_rgb[2] = 0.0f;
    
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].scale_factor = 1.0f;
    
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz_offset[0] = 0.0f;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz_offset[1] = 0.0f;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz_offset[2] = 0.0f;
    
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz_angle[0] = 0.0f;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz_angle[1] = 0.0f;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz_angle[2] = 0.0f;
    
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz_multiplier[0] = 1.0f;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz_multiplier[1] = 1.0f;
    frame_data->polygon_collection->polygons[
        frame_data->polygon_collection->size].xyz_multiplier[2] = 1.0f;
    
    frame_data->vertices[frame_data->vertices_size].texturearray_i =
        -1;
    frame_data->vertices[frame_data->vertices_size].texture_i = -1;
    frame_data->vertices[frame_data->vertices_size].polygon_i =
        (int)frame_data->polygon_collection->size;
    frame_data->vertices[frame_data->vertices_size].color[0] = 0.0f;
    frame_data->vertices[frame_data->vertices_size].color[1] =
        is_last_clicked ?
            ((platform_get_current_time_microsecs() / 25000) % 80) * 0.01f :
            1.0f;
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
    #ifndef LOGGER_IGNORE_ASSERTS
    if (
        window_globals->visual_debug_last_clicked_touchable_id < 0 &&
        window_globals->visual_debug_mode)
    {
        return;
    }
    
    for (uint32_t zp_i = 0; zp_i < zpolygons_to_render->size; zp_i++) {
        if (zpolygons_to_render->cpu_data[zp_i].touchable_id >= 0) {
            is_last_clicked =
                window_globals->visual_debug_last_clicked_touchable_id ==
                    zpolygons_to_render->cpu_data[zp_i].touchable_id;
            if (window_globals->visual_debug_mode ||
                is_last_clicked)
            {
                float left =
                    zpolygons_to_render->gpu_data[zp_i].xyz[0] -
                    (zpolygons_to_render->cpu_data[zp_i].hitbox_width / 2);
                float right =
                    zpolygons_to_render->gpu_data[zp_i].xyz[0] +
                    (zpolygons_to_render->cpu_data[zp_i].hitbox_width / 2);
                float top =
                    zpolygons_to_render->gpu_data[zp_i].xyz[1] +
                    (zpolygons_to_render->cpu_data[zp_i].hitbox_height / 2);
                float bottom =
                    zpolygons_to_render->gpu_data[zp_i].xyz[1] -
                    (zpolygons_to_render->cpu_data[zp_i].hitbox_height / 2);
                float front =
                    zpolygons_to_render->gpu_data[zp_i].xyz[2] -
                    (zpolygons_to_render->cpu_data[zp_i].hitbox_depth / 2);
                float back =
                    zpolygons_to_render->gpu_data[zp_i].xyz[2] +
                    (zpolygons_to_render->cpu_data[zp_i].hitbox_depth / 2);
                
                zVertex topleftfront;
                topleftfront.x = left;
                topleftfront.y = top;
                topleftfront.z = front;
                x_rotate_zvertex(
                    &topleftfront,
                    -zpolygons_to_render->gpu_data[zp_i].xyz_angle[0]);
                y_rotate_zvertex(
                    &topleftfront,
                    -zpolygons_to_render->gpu_data[zp_i].xyz_angle[1]);
                z_rotate_zvertex(
                    &topleftfront,
                    -zpolygons_to_render->gpu_data[zp_i].xyz_angle[2]);
                
                zVertex rightbottomback;
                rightbottomback.x = right;
                rightbottomback.y = bottom;
                rightbottomback.z = back;
                x_rotate_zvertex(
                    &rightbottomback, -zpolygons_to_render->gpu_data[zp_i].
                        xyz_angle[0]);
                y_rotate_zvertex(
                    &rightbottomback, -zpolygons_to_render->gpu_data[zp_i].
                        xyz_angle[1]);
                z_rotate_zvertex(
                    &rightbottomback, -zpolygons_to_render->gpu_data[zp_i].
                        xyz_angle[2]);
                
                // left top front -> right top front
                add_point_vertex(
                    frame_data,
                    topleftfront.x, topleftfront.y, topleftfront.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                add_point_vertex(
                    frame_data,
                    rightbottomback.x, topleftfront.y, topleftfront.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                
                // left bottom front -> right bottom front
                add_point_vertex(
                    frame_data,
                    topleftfront.x, rightbottomback.y, topleftfront.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                add_point_vertex(
                    frame_data,
                    rightbottomback.x, rightbottomback.y, topleftfront.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                
                // left top back -> right top back
                add_point_vertex(
                    frame_data,
                    topleftfront.x, topleftfront.y, rightbottomback.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                add_point_vertex(
                    frame_data,
                    rightbottomback.x, topleftfront.y, rightbottomback.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                
                // left bottom back -> right bottom back
                add_point_vertex(
                    frame_data,
                    topleftfront.x, rightbottomback.y, rightbottomback.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                add_point_vertex(
                    frame_data,
                    rightbottomback.x, rightbottomback.y, rightbottomback.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                
                // left top front -> left top back
                add_point_vertex(
                    frame_data,
                    topleftfront.x, topleftfront.y, topleftfront.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                add_point_vertex(
                    frame_data,
                    topleftfront.x, topleftfront.y, rightbottomback.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                
                // right top front -> right top back
                add_point_vertex(
                    frame_data,
                    rightbottomback.x, topleftfront.y, topleftfront.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                add_point_vertex(
                    frame_data,
                    rightbottomback.x, topleftfront.y, rightbottomback.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                
                // left bottom front -> left bottom back
                add_point_vertex(
                    frame_data,
                    topleftfront.x, rightbottomback.y, topleftfront.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                add_point_vertex(
                    frame_data,
                    topleftfront.x, rightbottomback.y, rightbottomback.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                
                // right bottom front -> right bottom back
                add_point_vertex(
                    frame_data,
                    rightbottomback.x, rightbottomback.y, topleftfront.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                add_point_vertex(
                    frame_data,
                    rightbottomback.x, rightbottomback.y, rightbottomback.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                
                // left top front -> left bottom front
                add_point_vertex(
                    frame_data,
                    topleftfront.x, topleftfront.y, topleftfront.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                add_point_vertex(
                    frame_data,
                    topleftfront.x, rightbottomback.y, topleftfront.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                
                // right top front -> right bottom front
                add_point_vertex(
                    frame_data,
                    rightbottomback.x, topleftfront.y, topleftfront.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                add_point_vertex(
                    frame_data,
                    rightbottomback.x, rightbottomback.y, topleftfront.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                
                // left top back -> left bottom back
                add_point_vertex(
                    frame_data,
                    topleftfront.x, topleftfront.y, rightbottomback.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                add_point_vertex(
                    frame_data,
                    topleftfront.x, rightbottomback.y, rightbottomback.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                
                // right top back -> right bottom back
                add_point_vertex(
                    frame_data,
                    rightbottomback.x, topleftfront.y, rightbottomback.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
                add_point_vertex(
                    frame_data,
                    rightbottomback.x, rightbottomback.y, rightbottomback.z,
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
            }
        }
    }
    #endif
}

inline static void zpolygons_to_triangles(
    GPUDataForSingleFrame * frame_data)
{
    uint32_t * next_workload_size = &frame_data->vertices_size;
    GPUPolygonCollection * gpu_polygons = frame_data->polygon_collection;
    
    assert(gpu_polygons->size == 0);
    assert(*next_workload_size == 0);
    
    memcpy(
        /* void * dest: */
            frame_data->polygon_collection,
        /* const void * src: */
            &zpolygons_to_render->gpu_data,
        /* size_t n: */
            sizeof(GPUPolygon) * zpolygons_to_render->size);
    frame_data->polygon_collection->size = zpolygons_to_render->size;
    
    log_assert(gpu_polygons->size <= zpolygons_to_render->size);
    log_assert(zpolygons_to_render->size < MAX_POLYGONS_PER_BUFFER);
    log_assert(gpu_polygons->size < MAX_POLYGONS_PER_BUFFER);
    
    for (
        int32_t cpu_zp_i = 0;
        cpu_zp_i < (int32_t)zpolygons_to_render->size;
        cpu_zp_i++)
    {
        if (
            zpolygons_to_render->cpu_data[cpu_zp_i].deleted ||
            !zpolygons_to_render->cpu_data[cpu_zp_i].visible ||
            zpolygons_to_render->cpu_data[cpu_zp_i].mesh_id < 0 ||
            (uint32_t)zpolygons_to_render->cpu_data[cpu_zp_i].mesh_id >=
                all_mesh_summaries_size)
        {
            continue;
        }
        
        int32_t mesh_id = zpolygons_to_render->cpu_data[cpu_zp_i].mesh_id;
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
            frame_data->vertices[*next_workload_size].polygon_i = cpu_zp_i;
            // (int)gpu_polygons->size; // this is not the same as cpu_zp_i!!!
            
            uint32_t mat_i = all_mesh_vertices[vert_i].parent_material_i;
            log_assert(
                mat_i < zpolygons_to_render->cpu_data[cpu_zp_i].vertex_materials_size);
            
            frame_data->vertices[*next_workload_size].color[0] =
                zpolygons_to_render->cpu_data[cpu_zp_i].vertex_materials[mat_i].color[0];
            frame_data->vertices[*next_workload_size].color[1] =
                zpolygons_to_render->cpu_data[cpu_zp_i].vertex_materials[mat_i].color[1];
            frame_data->vertices[*next_workload_size].color[2] =
                zpolygons_to_render->cpu_data[cpu_zp_i].vertex_materials[mat_i].color[2];
            frame_data->vertices[*next_workload_size].color[3] =
                zpolygons_to_render->cpu_data[cpu_zp_i].vertex_materials[mat_i].color[3];
            
            frame_data->vertices[*next_workload_size].texture_i =
                zpolygons_to_render->cpu_data[cpu_zp_i].
                    vertex_materials[mat_i].texture_i;
            frame_data->vertices[*next_workload_size].texturearray_i =
                zpolygons_to_render->cpu_data[cpu_zp_i].vertex_materials[mat_i].
                    texturearray_i;
            *next_workload_size += 1;
            assert(*next_workload_size < MAX_VERTICES_PER_BUFFER);
        }
        
        gpu_polygons->size += 1;
    }
    
    frame_data->first_line_i = frame_data->vertices_size;
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
    
    log_assert(zpolygons_to_render->size < MAX_POLYGONS_PER_BUFFER);
    
    zpolygons_to_triangles(frame_data);
    
    if (application_running) {
        add_particle_effects_to_workload(frame_data, elapsed_nanoseconds);
        
        //
        //        add_shatter_effects_to_workload(
        //            next_gpu_workload,
        //            next_workload_size,
        //            lights_for_gpu,
        //            elapsed_nanoseconds);
    }
    
    zpolygon_hitboxes_to_lines(
        frame_data);
    
    if (window_globals->wireframe_mode) {
        frame_data->first_line_i = 0;
    }
}
