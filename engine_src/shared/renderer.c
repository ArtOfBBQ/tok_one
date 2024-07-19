#include "renderer.h"

static uint32_t renderer_initialized = false;

void init_renderer(void) {
    renderer_initialized = true;
    
    camera.xyz[0]       = 0.0f;
    camera.xyz[1]       = 0.0f;
    camera.xyz[2]       = 0.0f;
    camera.xyz_angle[0] = 0.0f;
    camera.xyz_angle[1] = 0.0f;
    camera.xyz_angle[2] = 0.0f;
}

static bool32_t is_last_clicked = false;

static void add_line_vertex(
    GPUDataForSingleFrame * frame_data,
    const float xyz[3],
    const float ignore_camera)
{
    log_assert(frame_data->line_vertices != NULL);
    
    if (frame_data->line_vertices_size >= MAX_LINE_VERTICES) {
        return;
    }
    
    memcpy(
        &frame_data->line_vertices[frame_data->line_vertices_size].xyz,
        xyz,
        sizeof(float) * 3);
    
    frame_data->line_vertices[frame_data->line_vertices_size].ignore_camera =
        ignore_camera;
    
    frame_data->line_vertices_size += 1;
}

static void add_point_vertex(
    GPUDataForSingleFrame * frame_data,
    const float xyz[3],
    const float ignore_camera)
{
    log_assert(frame_data->point_vertices != NULL);
    
    if (frame_data->point_vertices_size >= MAX_POINT_VERTICES) {
        return;
    }
    
    memcpy(
        &frame_data->point_vertices[frame_data->point_vertices_size].xyz,
        xyz,
        sizeof(float) * 3);
    
    frame_data->point_vertices[frame_data->point_vertices_size].ignore_camera =
        ignore_camera;
    
    frame_data->point_vertices_size += 1;
}

inline static void draw_bounding_sphere(
    GPUDataForSingleFrame * frame_data,
    const float xyz[3],
    const float xyz_offset[3],
    const float sphere_radius,
    const float ignore_camera)
{
    float center_xyz[3];
    memcpy(center_xyz, xyz, sizeof(float) * 3);
    
    center_xyz[0] += xyz_offset[0];
    center_xyz[1] += xyz_offset[1];
    center_xyz[2] += xyz_offset[2];
    
    float cur_point[3];
    for (float x_angle = 0.2f; x_angle < 6.28f; x_angle += 0.75f)
    {
        for (float y_angle = 0.0f; y_angle < 6.28f; y_angle += 0.75f)
        {
            cur_point[0] = 0.0f;
            cur_point[1] = 0.0f;
            cur_point[2] = sphere_radius;
            x_rotate_f3(cur_point, x_angle);
            y_rotate_f3(cur_point, y_angle);
            
            cur_point[0] += center_xyz[0];
            cur_point[1] += center_xyz[1];
            cur_point[2] += center_xyz[2];
            
            add_point_vertex(frame_data, cur_point, ignore_camera);
        }
    }
}

inline static void zpolygon_vertices_to_points(
    GPUDataForSingleFrame * frame_data)
{
    for (uint32_t zp_i = 0; zp_i < zpolygons_to_render->size; zp_i++) {
        if (zpolygons_to_render->cpu_data[zp_i].touchable_id >= 0) {
            int32_t mesh_id = zpolygons_to_render->cpu_data[zp_i].mesh_id;
            
            int32_t tail_i =
                all_mesh_summaries[mesh_id].vertices_head_i +
                all_mesh_summaries[mesh_id].vertices_size;
            
            float transformed_triangle[9];
            
            for (
                int32_t triangle_lv_i = all_mesh_summaries[mesh_id].
                    vertices_head_i;
                triangle_lv_i < tail_i;
                triangle_lv_i += 3)
            {
                zpolygon_get_transformed_triangle_vertices(
                    /* const zPolygonCPU * cpu_data: */
                        &zpolygons_to_render->cpu_data[zp_i],
                    /* const GPUPolygon * gpu_data: */
                        &zpolygons_to_render->gpu_data[zp_i],
                    /* const unsigned int locked_vertex_i: */
                        triangle_lv_i,
                    /* float * vertices_recipient_f9: */
                        transformed_triangle);
                
                add_point_vertex(frame_data, transformed_triangle, false);
                add_point_vertex(frame_data, transformed_triangle + 3, false);
                add_point_vertex(frame_data, transformed_triangle + 6, false);
            }
        }
    }
}

inline static void zpolygon_hitboxes_to_lines(
    GPUDataForSingleFrame * frame_data)
{
    for (uint32_t zp_i = 0; zp_i < zpolygons_to_render->size; zp_i++) {
        if (zpolygons_to_render->cpu_data[zp_i].touchable_id >= 0) {
            draw_bounding_sphere(
                /* GPUDataForSingleFrame * frame_data: */
                    frame_data,
                /* const float xyz[3]: */
                    zpolygons_to_render->gpu_data[zp_i].xyz,
                /* const float xyz_offset[3]: */
                    zpolygons_to_render->gpu_data[zp_i].xyz_offset,
                /* const float sphere_radius: */
                    zpolygons_to_render->cpu_data[zp_i].boundsphere_radius,
                /* const float ignore_camera: */
                    zpolygons_to_render->gpu_data[zp_i].ignore_camera);
        }
    }
}

inline static void add_alphablending_zpolygons_to_workload(
    GPUDataForSingleFrame * frame_data)
{
    frame_data->first_alphablend_i = frame_data->vertices_size;
    
    // Copy all vertices that do use alpha blending
    for (
        int32_t cpu_zp_i = 0;
        cpu_zp_i < (int32_t)zpolygons_to_render->size;
        cpu_zp_i++)
    {
        if (
            zpolygons_to_render->cpu_data[cpu_zp_i].deleted ||
            !zpolygons_to_render->cpu_data[cpu_zp_i].visible ||
            !zpolygons_to_render->cpu_data[cpu_zp_i].committed ||
            !zpolygons_to_render->cpu_data[cpu_zp_i].alpha_blending_enabled)
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
            vert_i += 1)
        {
            frame_data->vertices[frame_data->vertices_size].locked_vertex_i =
                vert_i;
            frame_data->vertices[frame_data->vertices_size].polygon_i =
                cpu_zp_i;
            frame_data->vertices_size += 1;
            log_assert(frame_data->vertices_size < ALL_LOCKED_VERTICES_SIZE);
        }
    }
}

inline static void add_opaque_zpolygons_to_workload(
    GPUDataForSingleFrame * frame_data)
{
    assert(frame_data->polygon_collection->size == 0);
    assert(frame_data->vertices_size == 0);
    
    memcpy(
        /* void * dest: */
            frame_data->polygon_collection,
        /* const void * src: */
            zpolygons_to_render->gpu_data,
        /* size_t n: */
            sizeof(GPUPolygon) * zpolygons_to_render->size);
    frame_data->polygon_collection->size = zpolygons_to_render->size;
    
    log_assert(
        frame_data->polygon_collection->size <= zpolygons_to_render->size);
    log_assert(
        zpolygons_to_render->size < MAX_POLYGONS_PER_BUFFER);
    log_assert(
        frame_data->polygon_collection->size < MAX_POLYGONS_PER_BUFFER);
    
    memcpy(
        /* void *__dst: */
            frame_data->polygon_materials,
        /* const void *__src: */
            zpolygons_to_render->gpu_materials,
        /* size_t __n: */
            sizeof(GPUPolygonMaterial) *
                MAX_MATERIALS_PER_POLYGON *
                zpolygons_to_render->size);
    
    for (
        int32_t cpu_zp_i = 0;
        cpu_zp_i < (int32_t)zpolygons_to_render->size;
        cpu_zp_i++)
    {
        if (
            zpolygons_to_render->cpu_data[cpu_zp_i].deleted ||
            !zpolygons_to_render->cpu_data[cpu_zp_i].visible ||
            !zpolygons_to_render->cpu_data[cpu_zp_i].committed ||
            zpolygons_to_render->cpu_data[cpu_zp_i].alpha_blending_enabled)
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
            vert_i += 1)
        {
            frame_data->vertices[frame_data->vertices_size].locked_vertex_i =
                vert_i;
            frame_data->vertices[frame_data->vertices_size].polygon_i =
                cpu_zp_i;
            frame_data->vertices_size += 1;
            log_assert(frame_data->vertices_size < ALL_LOCKED_VERTICES_SIZE);
        }
    }
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
    
    add_opaque_zpolygons_to_workload(frame_data);
    
    if (application_running) {
        add_particle_effects_to_workload(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* uint64_t elapsed_nanoseconds: */
                elapsed_nanoseconds,
            /* const uint32_t alpha_blending: */
                false);
        
        add_lineparticle_effects_to_workload(
            frame_data,
            elapsed_nanoseconds,
            false);
    }
    
    frame_data->first_alphablend_i = frame_data->vertices_size;
    
    add_alphablending_zpolygons_to_workload(frame_data);
    
    add_particle_effects_to_workload(
        /* GPUDataForSingleFrame * frame_data: */
            frame_data,
        /* uint64_t elapsed_nanoseconds: */
            elapsed_nanoseconds,
        /* const uint32_t alpha_blending: */
            true);
    
    add_lineparticle_effects_to_workload(
            frame_data,
            elapsed_nanoseconds,
            true);
        
    add_points_and_lines_to_workload(frame_data);
    
    if (application_running && window_globals->draw_hitboxes) {
        zpolygon_hitboxes_to_lines(frame_data);
    }
    
    if (application_running && window_globals->draw_vertices) {
        zpolygon_vertices_to_points(frame_data);
    }
    
    if (application_running && window_globals->draw_clickray) {
        add_line_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float xyz[3]: */
                window_globals->last_clickray_origin,
            /* const float ignore_camera: */
                0.0f);
        
        float clickray_end[3];
        memcpy(
            clickray_end,
            window_globals->last_clickray_origin,
            sizeof(float) * 3);
        clickray_end[0] += window_globals->last_clickray_direction[0];
        clickray_end[1] += window_globals->last_clickray_direction[1];
        clickray_end[2] += window_globals->last_clickray_direction[2];
        add_line_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float xyz[3]: */
                clickray_end,
            /* const float ignore_camera: */
                0.0f);
        
        add_point_vertex(
            /* GPUDataForSingleFrame * frame_data: */
                frame_data,
            /* const float * xyz: */
                window_globals->last_clickray_collision,
            /* const float ignore_camera: */
                false);
    }
}
